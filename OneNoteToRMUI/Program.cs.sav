using System.CommandLine;

namespace OneNoteToRMUI
{
    internal static class Program
    {
        /// <summary>
        ///  The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Length != 0)
            {
                // Command line mode
                RootCommand rootCommand = new("Convert documents between ReMarkable and OneNote");
                rootCommand.Hidden = false;

                Option<string> InputFile = new("--Input", "-I")
                {
                    Description = "Input Document (Use quotes to enclose spaces)",
                    Required = true,
                    Recursive = true,
                    HelpName = "\"Document Name\""
                };

                Option<string> OutputFile = new("--Output", "-O")
                {
                    Description = "Output Document (Use quotes to enclose spaces)",
                    Required = true,
                    Recursive = true,
                    HelpName = "\"Document Name\""
                };

                rootCommand.Options.Add(InputFile);
                rootCommand.Options.Add(OutputFile);

//                Command Mode = new("-M", "Operational Mode: ");
//                rootCommand.Subcommands.Add(Mode);

                Command R2O = new("-MR", "Remarkable to OneNote");
                rootCommand.Subcommands.Add(R2O);
                Command O2R = new("-MO", "OneNote to Remarkable");
                rootCommand.Subcommands.Add(O2R);
                Command T = new("-MT", "Timed mode (Will do nothing if neither is updated since last T mode invoked)");
                rootCommand.Subcommands.Add(T);
                Command L = new("-ML", "Loop timed mode");
                rootCommand.Subcommands.Add(L);

                Option<int> LoopTime = new("--Loop", "-L")
                {
                    Description = "Loop Time (in seconds)",
                    DefaultValueFactory = ParseResult => 60
                };
//                rootCommand.Options.Add(LoopTime);
                L.Options.Add(LoopTime);

                R2O.SetAction(parseResult => {
                    string? InputFileName = parseResult.GetValue(InputFile);
                    string? OutputFileName = parseResult.GetValue(OutputFile);
                    int loopTime = parseResult.GetValue(LoopTime);
                    DoCommandLine(parseResult.CommandResult.Command.Name, InputFileName, OutputFileName, loopTime);
                });
                O2R.SetAction(parseResult => {
                    string? InputFileName = parseResult.GetValue(InputFile);
                    string? OutputFileName = parseResult.GetValue(OutputFile);
                    int loopTime = parseResult.GetValue(LoopTime);
                    DoCommandLine(parseResult.CommandResult.Command.Name, InputFileName, OutputFileName, loopTime);
                });
                T.SetAction(parseResult => {
                    string? InputFileName = parseResult.GetValue(InputFile);
                    string? OutputFileName = parseResult.GetValue(OutputFile);
                    int loopTime = parseResult.GetValue(LoopTime);
                    DoCommandLine(parseResult.CommandResult.Command.Name, InputFileName, OutputFileName, loopTime);
                });
                L.SetAction(parseResult => {
                    string? InputFileName = parseResult.GetValue(InputFile);
                    string? OutputFileName = parseResult.GetValue(OutputFile);
                    int loopTime = parseResult.GetValue(LoopTime);
                    DoCommandLine(parseResult.CommandResult.Command.Name, InputFileName, OutputFileName, loopTime);
                });

                ParseResult parseResult = rootCommand.Parse(args);
                parseResult.Invoke();
            }
            else
            {
                DllWrapper.DLLLog("Program", "INTERACTIVE MODE", DllWrapper.LogLevel.INFO);

                // To customize application configuration such as set high DPI settings or default font,
                // see https://aka.ms/applicationconfiguration.
                ApplicationConfiguration.Initialize();
                Application.Run(new OneNoteToRMUI());
            }
        }

        private static int DoCommandLine(string Mode, string? Input, string? Output, int? LoopTime)
        {
            DllWrapper.DLLSetLogListbox(null);
            DllWrapper.DLLLog("Program", "Processing Command Line", DllWrapper.LogLevel.INFO);

            if (Input is null || Output is null)
            {
                return 0;
            }

            DllWrapper? RMDoc, OneDoc;

            switch (Mode)
            {
                case "-MR":
                    {
                        RMDoc = new(DllWrapper.PageType.TO_ONE_RM_PAGE);
                        OneDoc = new(DllWrapper.PageType.WINDOW_ONE_PAGE);
                        int Pages = RMDoc.Load(Input);
                        for (int i =0; i < Pages; i++)
                            RMDoc.Convert(OneDoc, i);
                        OneDoc.Save(Output);
                        break;
                    }
                case "-MO":
                    {
                        OneDoc = new(DllWrapper.PageType.TO_RM_ONE_PAGE);
                        RMDoc = new(DllWrapper.PageType.WINDOW_RM_PAGE);
                        int Pages = OneDoc.Load(Input);
                        for (int i = 0; i < Pages; i++)
                            OneDoc.Convert(RMDoc, i);
                        RMDoc.Save(Output);
                        break;
                    }
                case "-MT":
                    {
                        DoCheck(Input, Output);
                        break;
                    }
                case "-ML":
                    {
                        while (true)
                        {
                            DoCheck(Input, Output);
                            DllWrapper.DLLLog("Program", $"Pausing for {LoopTime} secs...", DllWrapper.LogLevel.INFO);
                            Thread.Sleep(1000 * (LoopTime ?? 60));
                        }

                    }
                default:
                    break;
            }

            return 0;
        }

        public static void DoCheck(string RMFilename, string ONEFilename)
        {
            DateTime RMDT = DateTime.MinValue, OneDT = DateTime.MinValue;
            DllWrapper RMDoc, OneDoc;
            int RMPages = 0, ONEPages = 0;

            RMDoc = new(DllWrapper.PageType.TO_ONE_RM_PAGE); 
            RMPages = RMDoc.Load(RMFilename);
            RMDT = RMDoc.DocDateTime();
            DllWrapper.DLLLog("Program", "RM DOC DT:" + RMDT.ToString(), DllWrapper.LogLevel.INFO);

            OneDoc = new(DllWrapper.PageType.TO_RM_ONE_PAGE);
            ONEPages = OneDoc.Load(ONEFilename);
            OneDT = OneDoc.DocDateTime();
            DllWrapper.DLLLog("Program", "ONE DOC DT:" + OneDT.ToString(), DllWrapper.LogLevel.INFO);

            string LastCheckKey = RMFilename + ONEFilename;
            string LastCheckValue = DllWrapper.GetIniSetting("TimedUpdate", LastCheckKey);
            DateTime LastCheck = DateTime.MinValue;
            if (LastCheckValue != "")
            {
                try
                {
                    LastCheck = DateTime.Parse(LastCheckValue);
                }
                catch (Exception)
                {
                    // do nothing... we have min value
                    LastCheck = DateTime.MinValue;
                }
            }

            if (RMDT > OneDT)
            {
                DllWrapper.DLLLog("Program", "... RM is Later", DllWrapper.LogLevel.INFO);
                if (RMDT > LastCheck)
                {
                    OneDoc = new(DllWrapper.PageType.WINDOW_ONE_PAGE);
                    for (int i = 0; i < RMPages; i++)
                    {
                        RMDoc?.Convert(OneDoc, i);
                    }
                    OneDoc.Save(ONEFilename);
                }
                else
                {
                    DllWrapper.DLLLog("Program", "... but not changed", DllWrapper.LogLevel.INFO);
                }
            }
            else
            {
                DllWrapper.DLLLog("Program", "... ONE is Later", DllWrapper.LogLevel.INFO);
                if (OneDT > LastCheck)
                {
                    RMDoc = new(DllWrapper.PageType.WINDOW_RM_PAGE);
                    for (int i = 0; i < ONEPages; i++)
                    {
                        OneDoc?.Convert(RMDoc, i);
                    }
                    RMDoc.Save(RMFilename);
                }
                else
                {
                    DllWrapper.DLLLog("Program", "... but not changed", DllWrapper.LogLevel.INFO);
                }

            }
            DllWrapper.WriteIniSetting("TimedUpdate", LastCheckKey, DateTime.UtcNow.ToString());

        } // function DoCheck

    }
}