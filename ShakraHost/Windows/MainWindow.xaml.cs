using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using Un4seen.Bass;
using Un4seen.Bass.AddOn.Midi;

namespace ShakraHost
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 
    public class ControlWriter : TextWriter
    {
        private TextBox TX;

        public ControlWriter(TextBox TextBox)
        {
            this.TX = TextBox;
        }

        public override void Write(char value)
        {
            TX.Dispatcher.Invoke(
             DispatcherPriority.Normal,
             new Action(() => {
                 TX.AppendText(value.ToString());
             }));
        }

        public override void Write(string value)
        {
            TX.Dispatcher.Invoke(
                DispatcherPriority.Normal,
                new Action(() => {
                    TX.AppendText(value);
                }));
        }

        public override Encoding Encoding
        {
            get { return Encoding.ASCII; }
        }
    }

    public class ShakraDLL
    {
        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_CP")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CreatePipe(ushort Pipe, int Size);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PSE")]
        public static extern uint ParseShortEvent(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PLE")]
        public static extern unsafe uint ParseLongEvent(ushort Pipe, IntPtr PEvent);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_FLE")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool FreeLongEvent(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_RRHIN")]
        public static extern void ResetReadHeadsIfNeeded(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GSRHP")]
        public static extern int GetShortReadHeadPos(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GSWHP")]
        public static extern int GetShortWriteHeadPos(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_BC")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool PerformBufferCheck(ushort Pipe);
    }

    public class MIDIEvent
    {
        private uint WholeEvent = 0;

        private byte Status = 0;
        private byte LRS = 0;

        private byte EventType = 0;
        private byte Channel = 0;

        private byte Param1 = 0;
        private byte Param2 = 0;

        private byte Secret = 0;

        public MIDIEvent(uint DWORD)
        {
            SetNewEvent(DWORD);
        }

        public void SetNewEvent(uint DWORD)
        {
            WholeEvent = DWORD;

            Status = (byte)(WholeEvent & 0xFF);
            if ((Status & 0x80) != 0)
                LRS = Status;
            else
                WholeEvent = WholeEvent << 8 | LRS;

            EventType = (byte)(LRS & 0xF0);
            Channel = (byte)(LRS & 0x0F);

            Param1 = (byte)(WholeEvent >> 8);
            Param2 = (byte)(WholeEvent >> 16);

            Secret = (byte)(WholeEvent >> 24);
        }

        public uint GetWholeEvent() { return WholeEvent; }

        public byte GetStatus() { return Status; }

        public byte GetEventType() { return EventType; }
        public byte GetChannel() { return Channel; }

        public int GetParams() { return Param1 | Param2 << 8; }
        public byte GetFirstParam() { return Param1; }
        public byte GetSecondParam() { return Param2; }

        public byte GetSecret() { return Secret; }
    }

    public class PipesData
    {
        public int Handle = 0;
        public Thread BASST = null;
        public BASS_MIDI_FONT[] SF;
        public ushort PipeID = 0;
    }

    public partial class MainWindow : Window
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool AllocConsole();

        [DllImport("ntdll.dll", SetLastError = true)]
        static extern bool NtDelayExecution(in bool Alertable, in Int64 DelayInterval);

        PipesData[] Pipes = new PipesData[4];
        bool[] PipesKillSwitches = new bool[4];
        DispatcherTimer DTimer = new DispatcherTimer();

        public void Test()
        {
            ulong A = 0;

            Thread.VolatileWrite(ref A, 2);
        }

        public MainWindow()
        {
            InitializeComponent();
            this.Closing += new CancelEventHandler(OnClosing);

            MessageBox.Show("waiting");

            try
            {
                StartThreads();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void OnClosing(object sender, CancelEventArgs e)
        {
            StopThreads();
        }

        private void DTimerTick(object sender, EventArgs e)
        {
            float A = 0.0f, B = 0.0f, C = 0.0f, D = 0.0f;

            int SRH1 = ShakraDLL.GetShortReadHeadPos(0), SWH1 = ShakraDLL.GetShortWriteHeadPos(0),
                SRH2 = ShakraDLL.GetShortReadHeadPos(1), SWH2 = ShakraDLL.GetShortWriteHeadPos(1),
                SRH3 = ShakraDLL.GetShortReadHeadPos(2), SWH3 = ShakraDLL.GetShortWriteHeadPos(2),
                SRH4 = ShakraDLL.GetShortReadHeadPos(3), SWH4 = ShakraDLL.GetShortWriteHeadPos(3);

            Bass.BASS_ChannelGetAttribute(Pipes[0].Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES_ACTIVE, ref A);
            Bass.BASS_ChannelGetAttribute(Pipes[1].Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES_ACTIVE, ref B);
            Bass.BASS_ChannelGetAttribute(Pipes[2].Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES_ACTIVE, ref C);
            Bass.BASS_ChannelGetAttribute(Pipes[3].Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES_ACTIVE, ref D);

            RH.Content = String.Format("SRH/SWH 1: {0:D6}/{1:D6} | SRH/SWH 2: {2:D6}/{3:D6} | SRH/SWH 3: {4:D6}/{5:D6} | SRH/SWH 4: {6:D6}/{7:D6}",
                SRH1.ToString(), SWH1.ToString(), SRH2.ToString(), SWH2.ToString(), SRH3.ToString(), SWH3.ToString(), SRH4.ToString(), SWH4.ToString());

            WH.Content = String.Format("V1: {0:D4} | V2: {1:D4} | V3: {2:D4} | V4: {3:D4}", Convert.ToInt32(A), Convert.ToInt32(B), Convert.ToInt32(C), Convert.ToInt32(D));

            CurBuf.Content = "N/A";
        }

        private void StartThreads()
        {
            BASS_INFO info = new BASS_INFO();

            Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATETHREADS, 0);
            Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATEPERIOD, 0);

            if (Bass.BASS_Init(-1, 48000, BASSInit.BASS_DEVICE_DEFAULT | BASSInit.BASS_DEVICE_LATENCY | BASSInit.BASS_DEVICE_DSOUND, IntPtr.Zero))
            {
                Bass.BASS_GetInfo(info);
                Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_BUFFER, info.minbuf - 10);

                for (ushort i = 0; i < Pipes.Length; i++)
                {
                    PipesData PipeD = new PipesData();
                    PipeD.PipeID = i;

                    Pipes[i] = PipeD;

                    Pipes[i].BASST = new Thread(BASSThread);
                    Pipes[i].BASST.Start(Pipes[i]);
                }

                DTimer.Tick += DTimerTick;
                DTimer.Interval = new TimeSpan(0, 0, 0, 0, 10);
            }
        }

        private void StopThreads()
        {
            for (ushort i = 0; i < Pipes.Length; i++)
            {
                PipesKillSwitches[i] = true;

                while (Pipes[i].BASST.IsAlive) /* Spin while the thread is still alive */;

                Bass.BASS_StreamFree(Pipes[i].Handle);

                PipesKillSwitches[i] = false;
            }

            Bass.BASS_Free();
        }

        private unsafe void BASSThread(object Pipe)
        {
            // Pipe
            PipesData TPipe;

            // Short
            BASS_MIDI_EVENT ev;
            MIDIEvent SEvent = new MIDIEvent(0x00000000);

            // Long
            IntPtr PEvent;
            uint PEventS = 0;

            try
            {
                TPipe = (PipesData)Pipe;
                PEvent = Marshal.AllocHGlobal(65536);

                TPipe.SF = new BASS_MIDI_FONT[1];
                TPipe.SF[0].font = BassMidi.BASS_MIDI_FontInit("test.sf2");
                TPipe.SF[0].preset = -1;
                TPipe.SF[0].bank = 0;

                TPipe.Handle = BassMidi.BASS_MIDI_StreamCreate(16, BASSFlag.BASS_SAMPLE_FLOAT, 0);
                BassMidi.BASS_MIDI_StreamSetFonts(TPipe.Handle, TPipe.SF, TPipe.SF.Length);
                BassMidi.BASS_MIDI_StreamLoadSamples(TPipe.Handle);

                Bass.BASS_ChannelSetAttribute(TPipe.Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES, 1000);
                Bass.BASS_ChannelPlay(TPipe.Handle, false);

                ShakraDLL.CreatePipe(TPipe.PipeID, 16384);
                DTimer.Start();

                while (!PipesKillSwitches[TPipe.PipeID])
                {
                    PEventS = ShakraDLL.ParseLongEvent(TPipe.PipeID, PEvent);
                    if (PEventS != 0)
                    {
                        byte[] MIDIHDR = new byte[PEventS];
                        Marshal.Copy(PEvent, MIDIHDR, 0, MIDIHDR.Length);
                        BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, MIDIHDR);
                    }

                    Bass.BASS_ChannelUpdate(TPipe.Handle, 0);
                    NtDelayExecution(false, -1);

                    if (!ShakraDLL.PerformBufferCheck(TPipe.PipeID))
                        continue;

                    int TempWH = ShakraDLL.GetShortWriteHeadPos(TPipe.PipeID);
                    do
                    {
                        SEvent.SetNewEvent(ShakraDLL.ParseShortEvent(TPipe.PipeID));
                        uint dwParam1 = SEvent.GetWholeEvent();
                        ShakraDLL.ResetReadHeadsIfNeeded(TPipe.PipeID);

                        switch (SEvent.GetEventType())
                        {
                            // Note OFF
                            case 0x80:
                                ev = new BASS_MIDI_EVENT(BASSMIDIEvent.MIDI_EVENT_NOTE, SEvent.GetFirstParam(), SEvent.GetChannel(), 0, 0);
                                break;
                            // Note ON
                            case 0x90:
                                ev = new BASS_MIDI_EVENT(BASSMIDIEvent.MIDI_EVENT_NOTE, SEvent.GetParams(), SEvent.GetChannel(), 0, 0);
                                break;
                            // Aftertouch
                            case 0xA0:
                                ev = new BASS_MIDI_EVENT(BASSMIDIEvent.MIDI_EVENT_KEYPRES, SEvent.GetParams(), SEvent.GetChannel(), 0, 0);
                                break;
                            // Instrument select
                            case 0xC0:
                                ev = new BASS_MIDI_EVENT(BASSMIDIEvent.MIDI_EVENT_PROGRAM, SEvent.GetFirstParam(), SEvent.GetChannel(), 0, 0);
                                break;
                            // Channel volume
                            case 0xD0:
                                ev = new BASS_MIDI_EVENT(BASSMIDIEvent.MIDI_EVENT_CHANPRES, SEvent.GetFirstParam(), SEvent.GetChannel(), 0, 0);
                                break;
                            // Control and pitch bends, let BASSMIDI handle the MSB/LSB hell...
                            case 0xE0:
                            case 0xB0:
                                BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, (IntPtr)(&dwParam1), 3);
                                continue;
                            default:
                                continue;
                        }

                        BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_STRUCT, new BASS_MIDI_EVENT[] { ev });
                    } while (ShakraDLL.GetShortReadHeadPos(TPipe.PipeID) != TempWH);
                }

                PipesKillSwitches[TPipe.PipeID] = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
