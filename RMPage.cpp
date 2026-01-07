namespace RMtoDOCX
{

    internal class RMPage
    {

        struct  RMHeader
        {
            fixed char header[43];
        };



        private ItemCollection log;

        public RMPage(ItemCollection Log)
        {
            this.log = Log;
        }
        public void Load(string FileName)
        {
            log.Add($"Opening {FileName.Substring(FileName.LastIndexOf(Path.DirectorySeparatorChar) + 1)}");

            using FileStream fs = new FileStream(FileName, FileMode.Open, FileAccess.Read);
            using (BinaryReader r = new BinaryReader(fs))
            {

                for (int i = 0; i < 11; i++)
                {
                    Console.WriteLine(r.ReadInt32());
                }
            }

        }
    }
}
