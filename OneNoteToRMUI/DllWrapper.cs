using System.Runtime.InteropServices;
using System.Text;

/*******************************************************************************

    DLLWrapper.cs

    Wrapper layer on DLL functions to encapsulate marshalling and provide 
        C# friendly API for other functions

    (C) David Poirier 2026

********************************************************************************/


namespace OneNoteToRMUI
{
    internal class DllWrapper(DllWrapper.PageType PT)
    {
        private const string ONTR_DLL_PATH = "OneNoteToRMDLL.dll";

        [StructLayout(LayoutKind.Explicit)]
        public struct Rect
        {
            [FieldOffset(0)] public int left;
            [FieldOffset(4)] public int top;
            [FieldOffset(8)] public int right;
            [FieldOffset(12)] public int bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct DrawDetailsParams
        {
            public IntPtr hDC;
            public Rect R;
        }


        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        private static extern string GetIniFile();

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern Int32 GetIniFileB(StringBuilder Buffer, Int32 BuffLen);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetLogListbox(IntPtr hWnd);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern void DoLog(string Class, string Msg, int Level);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern void SetToken(Int32 PageType, [MarshalAs(UnmanagedType.LPWStr)] String LoginCodeW);


        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern Int32 ListDocs(Int32 PageType, [MarshalAs(UnmanagedType.LPWStr)] StringBuilder Buffer, Int32 BuffLen);


        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateEmptyDoc(Int32 PageType);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern int LoadDoc(IntPtr Doc, String FileName);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern Int32 ConvertPageB(IntPtr Source, ref DrawDetailsParams DDP, int Page);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern Int32 ConvertPage(IntPtr Source, IntPtr Dest, int Page);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern int SaveDoc(IntPtr Doc, String FileName);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern UInt64 GetDocDateTime(IntPtr Doc);

        [DllImport(ONTR_DLL_PATH, CallingConvention = CallingConvention.Cdecl)]
        private static extern Int32 DeleteDoc(IntPtr Doc);


        [DllImport("kernel32.dll", EntryPoint = "GetPrivateProfileString")]
        private static extern int GetPrivateProfileString(string SectionName, string KeyName, string Default, StringBuilder Return_StringBuilder_Name, int Size, string FileName);

        [DllImport("kernel32.dll", EntryPoint = "WritePrivateProfileString")]
        private static extern long WritePrivateProfileString(string SectionName, string KeyName, string Value, string FileName);

        public enum PageType
        {
            WINDOW_RM_PAGE = 1,
            TO_ONE_RM_PAGE = 2,
            WINDOW_ONE_PAGE = 3,
            TO_RM_ONE_PAGE = 4
        }

        public enum LogLevel
        {
                VERBOSE = 0,
                DEBUG,
                INFO,
                WARNING,
                ERROR
        }

        /// /////////////////////////////////////////////////////////////////
        /// Public static functions - for general usage
        /// 
        public static void DLLSetLogListbox(Control? LB)
        {
            if (LB == null)
                SetLogListbox(0);
            else
                SetLogListbox(LB.Handle);
        }

        public static void DLLLog(string Class, string Msg, LogLevel Level)
        {
            DoLog(Class, Msg, (int)Level);
        }

        public static void DLLSetToken(PageType PT, String LoginCode)
        {
            SetToken((int)PT, LoginCode);
        }

        public static String DLLGetIniFile()
        {
            var SB = new StringBuilder(1024);
            _ = GetIniFileB(SB, 1023);
            return SB.ToString();
        }

        public static string[] DLLListDocs(PageType PT)
        {
            var SB = new StringBuilder(102400);
            var ret = ListDocs((int)PT, SB, 102399);
            if (ret != 0)
                throw new InvalidOperationException();

            return SB.ToString().Split("\n") ;
        }

        public static string GetIniSetting(string Section, string Key)
        {
            var SB = new StringBuilder(1024);
            _ = GetPrivateProfileString(Section, Key, "", SB, 1023, DLLGetIniFile());
            return SB.ToString();
        }

        public static void WriteIniSetting(string Section, string Key, string Value)
        {
            WritePrivateProfileString(Section, Key, Value, DLLGetIniFile());
        }


        /// /////////////////////////////////////////////////////////////////
        /// Public class functions - for general usage
        /// 
        private readonly IntPtr hDoc = CreateEmptyDoc((int)PT);

        ~DllWrapper()
        {
            if (hDoc != IntPtr.Zero)
                _ = DeleteDoc(hDoc);
        }

        public int Load(String FileName)
        {
            return LoadDoc(hDoc, FileName);
        }

        public Rect DrawPage(IntPtr hDC, int Page)
        {
            DrawDetailsParams DDP = new()
            {
                hDC = hDC
            };
            _ = ConvertPageB(hDoc, ref DDP, Page);
            return DDP.R;
        }

        public int Convert(DllWrapper Dest, int Page)
        { 
            return ConvertPage(hDoc, Dest.hDoc, Page);
        }

        public int Save(String FileName) 
        { 
            return SaveDoc(hDoc, FileName);
        }

        public DateTime DocDateTime()
        {
            UInt64 Time = GetDocDateTime(hDoc);
            DateTime DT = new(1970, 1, 1, 0, 0, 0, DateTimeKind.Local);
            DT = DT.AddSeconds(Time);

            return DT;
        }

    }
}
