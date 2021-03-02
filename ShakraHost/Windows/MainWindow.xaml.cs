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

        public void Test()
        {
            ulong A = 0;

            Thread.VolatileWrite(ref A, 2);
        }

        public MainWindow()
        {
            InitializeComponent();

            Console.SetOut(new ControlWriter(ConsoleOutput));
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
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void BASSThread(object Pipe)
        {
            try
            {
                PipesData TPipe = (PipesData)Pipe;
                uint Event = 0;
                byte[] EventBuf;

                TPipe.SF = new BASS_MIDI_FONT[1];
                TPipe.SF[0].font = BassMidi.BASS_MIDI_FontInit("test.sf2");

                Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATEPERIOD, 5);
                Bass.BASS_SetConfig(BASSConfig.BASS_CONFIG_UPDATETHREADS, 4);

                TPipe.Handle = BassMidi.BASS_MIDI_StreamCreate(16, BASSFlag.BASS_SAMPLE_FLOAT, 0);
                BassMidi.BASS_MIDI_StreamSetFonts(TPipe.Handle, TPipe.SF, TPipe.SF.Length);
                BassMidi.BASS_MIDI_StreamLoadSamples(TPipe.Handle);

                Bass.BASS_ChannelSetAttribute(TPipe.Handle, BASSAttribute.BASS_ATTRIB_MIDI_VOICES, 1000);
                Bass.BASS_ChannelPlay(TPipe.Handle, false);

                ShakraDLL.CreatePipe(TPipe.PipeID, 4096);

                while (true)
                {
                    NtDelayExecution(false, -1);

                    if (!ShakraDLL.PerformBufferCheck(TPipe.PipeID))
                        continue;

                    do
                    {
                        int Length = 3;
                        Event = ShakraDLL.ParseShortEvent(TPipe.PipeID);
                        ShakraDLL.ResetReadHeadIfNeeded(TPipe.PipeID);

                        switch (Event & 0xF0)
                        {
                            case 0x90:
                                BassMidi.BASS_MIDI_StreamEvent(TPipe.Handle, (int)(Event & 0xF), BASSMIDIEvent.MIDI_EVENT_NOTE, (int)(Event >> 8));
                                break;

                            case 0x80:
                                BassMidi.BASS_MIDI_StreamEvent(TPipe.Handle, (int)(Event & 0xF), BASSMIDIEvent.MIDI_EVENT_NOTE, (int)((Event & 0xFFFF00) >> 8));
                                break;

                            default:
                                if (((Event - 0x80) & 0xC0) == 0)
                                {
                                    EventBuf = new byte[] { (byte)Event, (byte)(Event >> 8), (byte)(Event >> 16), (byte)(Event >> 24) };
                                    BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, EventBuf);
                                    break;
                                }

                                if (((Event - 0x80) & 0xC0) == 0) Length = 2;
                                else if ((Event & 0xF0) == 0xF0)
                                {
                                    switch (Event & 0xF)
                                    {
                                        case 3:
                                            Length = 2;
                                            break;
                                        default:
                                            continue;
                                    }
                                }

                                EventBuf = new byte[] { 0, (byte)(Event >> 8), (byte)(Event >> 16), (byte)(Event >> 24) };
                                BassMidi.BASS_MIDI_StreamEvents(TPipe.Handle, BASSMIDIEventMode.BASS_MIDI_EVENTS_RAW, 0, EventBuf);
                                break;
                        }
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
