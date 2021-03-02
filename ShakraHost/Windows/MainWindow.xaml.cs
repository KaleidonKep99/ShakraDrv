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
        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_CP", CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CreatePipe(ushort Pipe, int Size);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PSE", CallingConvention = CallingConvention.StdCall)]
        public static extern uint ParseShortEvent(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PLE", CallingConvention = CallingConvention.StdCall)]
        public static extern int ParseLongEvent(ushort Pipe, [MarshalAs(UnmanagedType.LPStr)] ref string IIMidiHdr);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_RRHIN", CallingConvention = CallingConvention.StdCall)]
        public static extern void ResetReadHeadIfNeeded(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GRHP", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetReadHeadPos(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GWHP", CallingConvention = CallingConvention.StdCall)]
        public static extern int GetWriteHeadPos(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_BC", CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool PerformBufferCheck(ushort Pipe);
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
            RH.Content = String.Format("RH: {0}", ShakraDLL.GetReadHeadPos(0));
            WH.Content = String.Format("WH: {0}", ShakraDLL.GetWriteHeadPos(0));
            CurBuf.Content = String.Format("CurBuf: {0:X8}", ShakraDLL.ParseShortEvent(0));
        }

        private unsafe void BASSThread(object Pipe)
        {
            try
            {
                PipesData TPipe = (PipesData)Pipe;
                uint Event = 0;

                // Long event
                int LESize = 0;
                string LEvent = null;

                BASS_MIDI_EVENT BEvent = new BASS_MIDI_EVENT { };

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
                    NtDelayExecution(false, -1);

                    if (!ShakraDLL.PerformBufferCheck(TPipe.PipeID))
                        continue;

                    LESize = ShakraDLL.ParseLongEvent(TPipe.PipeID, ref LEvent);
                    if (LESize != -1 && LEvent != null)
                    {
                        BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, IntPtr.Parse(LEvent), LESize);
                    }

                    do
                    {             
                        Event = ShakraDLL.ParseShortEvent(TPipe.PipeID);
                        ShakraDLL.ResetReadHeadIfNeeded(TPipe.PipeID);

                        BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, (IntPtr)(&Event), 3);

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
