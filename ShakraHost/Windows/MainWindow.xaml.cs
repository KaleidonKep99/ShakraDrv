using System;
using System.Buffers.Binary;
using System.Collections.Generic;
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

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GRHP")]
        public static extern void GetReadHeadPos(ushort Pipe, out int ShortReadHead, out int LongReadHead);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GWHP")]
        public static extern void GetWriteHeadPos(ushort Pipe, out int ShortWriteHead, out int LongWriteHead);

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
        public byte GetSecondParam() { return Secret; }

        public byte GetSecret() { return Param2; }
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
        DispatcherTimer DTimer = new DispatcherTimer();

        public void Test()
        {
            ulong A = 0;

            Thread.VolatileWrite(ref A, 2);
        }

        public MainWindow()
        {
            InitializeComponent();

            MessageBox.Show("waiting");

            try
            {
                if (Bass.BASS_Init(-1, 48000, BASSInit.BASS_DEVICE_DEFAULT, IntPtr.Zero))
                {
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
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void DTimerTick(object sender, EventArgs e)
        {
            int SRH = 0, LRH = 0, SWH = 0, LWH = 0;

            ShakraDLL.GetReadHeadPos(0, out SRH, out LRH);
            ShakraDLL.GetWriteHeadPos(0, out SWH, out LWH);

            MIDIEvent SEvent = new MIDIEvent(ShakraDLL.ParseShortEvent(0));

            RH.Content = String.Format("SRH/SWH: {0:D4}/{1:D4}", SRH.ToString(), SWH.ToString());
            WH.Content = String.Format("LRH/LWH: {0:D4}/{1:D4}", LRH.ToString(), LWH.ToString());
            CurBuf.Content = String.Format("{0:X8} (Ch {1:X2}-{1:D3} | P1 {2:X2}-{2:D3} | P2 {3:X2}-{3:D3}) | Ps {4:X4}-{4:D5})",
                SEvent.GetWholeEvent(), SEvent.GetChannel(),
                SEvent.GetFirstParam(), SEvent.GetSecondParam(), SEvent.GetParams());
        }

        private unsafe void BASSThread(object Pipe)
        {
            // Short
            BASS_MIDI_EVENT ev;
            MIDIEvent SEvent = new MIDIEvent(0x00000000);

            // Long
            IntPtr PEvent = Marshal.AllocHGlobal(65536);
            uint PEventS = 0;

            try
            {
                PipesData TPipe = (PipesData)Pipe;

                TPipe.SF = new BASS_MIDI_FONT[1];
                TPipe.SF[0].font = BassMidi.BASS_MIDI_FontInit("test.sf2");
                TPipe.SF[0].preset = -1;
                TPipe.SF[0].bank = 0;

                Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATEPERIOD, 5);
                Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATETHREADS, 4);

                TPipe.Handle = BassMidi.BASS_MIDI_StreamCreate(16, BASSFlag.BASS_SAMPLE_FLOAT, 0);
                BassMidi.BASS_MIDI_StreamSetFonts(TPipe.Handle, TPipe.SF, TPipe.SF.Length);
                BassMidi.BASS_MIDI_StreamLoadSamples(TPipe.Handle);

                Bass.BASS_ChannelSetAttribute(TPipe.Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES, 1000);
                Bass.BASS_ChannelPlay(TPipe.Handle, false);

                ShakraDLL.CreatePipe(TPipe.PipeID, 4096);
                DTimer.Start();

                while (true)
                {
                    PEventS = ShakraDLL.ParseLongEvent(TPipe.PipeID, PEvent);
                    if (PEventS != 0)
                    {
                        byte[] MIDIHDR = new byte[PEventS];
                        Marshal.Copy(PEvent, MIDIHDR, 0, MIDIHDR.Length);
                        BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, MIDIHDR);
                    }

                    if (!ShakraDLL.PerformBufferCheck(TPipe.PipeID))
                    {
                        NtDelayExecution(false, -1);
                        continue;
                    }

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
                    } while (ShakraDLL.PerformBufferCheck(TPipe.PipeID));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
