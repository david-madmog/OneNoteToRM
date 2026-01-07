namespace RMtoDOCX
{
    internal class RMZipFile
    {
        const string WorkingDir = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\RMtoDOCX\\RMtoDOCX\\Working";
        private ItemCollection log;
        public Collection<RMPage> Pages = [];

        public RMZipFile(ItemCollection Log)
        {
            this.log = Log;
        }

        public void ExtractRMsFromZip(string FileName)
        {
            ZipFile.ExtractToDirectory(FileName, WorkingDir, true);
            var files = Directory.EnumerateFiles(WorkingDir, "*.rm", SearchOption.AllDirectories);
            foreach(string file in files)
            {
                RMPage Page = new RMPage(log);
                Page.Load(file);
                Pages.Add(Page);
            }

        }
    }
}
