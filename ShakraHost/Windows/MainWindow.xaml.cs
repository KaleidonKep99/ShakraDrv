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
        public static extern bool CreatePipe(out string Pipe, int Size);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PSE")]
        public static extern uint ParseShortEvent();

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_PLE")]
        public static extern unsafe uint ParseLongEvent(IntPtr PEvent);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_FLE")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool FreeLongEvent();

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_RRHIN")]
        public static extern void ResetReadHeadsIfNeeded(ushort Pipe);

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GRHP")]
        public static extern int GetReadHeadPos();

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_GWHP")]
        public static extern int GetWriteHeadPos();

        [DllImport("shakra.dll", SetLastError = true, EntryPoint = "SH_BC")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool PerformBufferCheck();
    }

    class KDMAPI
    {
        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int ReturnKDMAPIVer(ref Int32 Major, ref Int32 Minor, ref Int32 Build, ref Int32 Revision);

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int InitializeKDMAPIStream();

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int TerminateKDMAPIStream();

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void ResetKDMAPIStream();

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int IsKDMAPIAvailable();

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SendCustomEvent(uint eventtype, uint chan, uint param);

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SendDirectData(uint dwMsg);

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SendDirectDataNoBuf(uint dwMsg);

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SendDirectLongData(IntPtr IIMidiHdr);

        [DllImport("OmniMIDI.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern int SendDirectLongDataNoBuf(IntPtr IIMidiHdr);

        public static String KDMAPIVer = "Null";
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

    public class ShakraPipe
    {
        public Thread RenderThread = null;
        public string PipeID = "";
        public bool KillSwitch = false;
    }

    public partial class MainWindow : Window
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool AllocConsole();

        [DllImport("ntdll.dll", SetLastError = true)]
        static extern bool NtDelayExecution(in bool Alertable, in Int64 DelayInterval);

        ShakraPipe Pipe = new ShakraPipe();
        DispatcherTimer DTimer = new DispatcherTimer();

        public MainWindow()
        {
            InitializeComponent();
            this.Closing += new CancelEventHandler(OnClosing);

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
            RH.Content = String.Format("SRH/SWH: {0:D6}/{1:D6}",
                ShakraDLL.GetReadHeadPos(), ShakraDLL.GetWriteHeadPos());

            WH.Content = String.Format("pre-alpha");

            CurBuf.Content = "N/A";
        }

        private void StartThreads()
        {
            if (KDMAPI.InitializeKDMAPIStream() == 1)
            {
                Pipe.PipeID = "0";
                Pipe.RenderThread = new Thread(BASSThread);

                DTimer.Tick += DTimerTick;
                DTimer.Interval = new TimeSpan(0, 0, 0, 0, 10);

                Pipe.RenderThread.Start(Pipe);
            }
        }

        private void StopThreads()
        {
            Pipe.KillSwitch = true;

            while (Pipe.RenderThread.IsAlive) /* Spin while the thread is still alive */;

            Pipe.KillSwitch = false;
        }

        private unsafe void BASSThread(object Pipe)
        {
            // Pipe
            ShakraPipe TPipe;

            // Short
            MIDIEvent SEvent = new MIDIEvent(0x00000000);

            // Long
            IntPtr PEvent;
            uint PEventS = 0;

            try
            {
                TPipe = (ShakraPipe)Pipe;
                PEvent = Marshal.AllocHGlobal(65536);

                ShakraDLL.CreatePipe(out TPipe.PipeID, 16384);
                DTimer.Start();

                while (!TPipe.KillSwitch)
                {
                    PEventS = ShakraDLL.ParseLongEvent(PEvent);
                    if (PEventS != 0)
                        KDMAPI.SendDirectLongData(PEvent);

                    NtDelayExecution(false, -1);

                    if (!ShakraDLL.PerformBufferCheck())
                        continue;

                    int TempWH = ShakraDLL.GetWriteHeadPos();
                    do
                    {
                        SEvent.SetNewEvent(ShakraDLL.ParseShortEvent());
                        uint dwParam1 = SEvent.GetWholeEvent();
                        ShakraDLL.ResetReadHeadsIfNeeded(0);

                        // KDMAPI.SendDirectDataNoBuf(dwParam1);
                        KDMAPI.SendCustomEvent(SEvent.GetEventType(), SEvent.GetChannel(), (uint)SEvent.GetParams());
                    } while (ShakraDLL.GetReadHeadPos() != TempWH);
                }

                TPipe.KillSwitch = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }
    }
}
