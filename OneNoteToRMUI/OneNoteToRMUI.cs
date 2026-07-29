using static System.ComponentModel.Design.ObjectSelectorEditor;

namespace OneNoteToRMUI
{
    public partial class OneNoteToRMUI : Form
    {
        public OneNoteToRMUI()
        {
            InitializeComponent();
        }


        private void btnRMPreview_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                string FileName = SelectedNode.FullPath;
                IntPtr hDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_RM_PAGE);
                int Pages = DllWrapper.DLLLoadDoc(hDoc, FileName);
                PreviewForm Popup = new PreviewForm();
                Popup.hDoc = hDoc;
                Popup.SetPages(Pages);
                Popup.Text = FileName;
                Popup.Show();
            }
        }

        private void btnRMRefresh_Click(object sender, EventArgs e)
        {
            string[] strings = DllWrapper.DLLListDocs(DllWrapper.PageType.WINDOW_RM_PAGE);
            foreach (string s in strings)
            {
                if (s != String.Empty)
                {                //lstRM.Items.Add(s);
                    string[] Path = s.Split(tvRM.PathSeparator);
                    if (Path.Length == 1)
                        tvRM.Nodes.Add(s);
                    else
                    {
                        TreeNodeCollection CurrentNodes = tvRM.Nodes;
                        foreach (string P in Path)
                        {
                            int i = CurrentNodes.IndexOfKey(P);
                            if (i == -1) // Not found
                            {
                                TreeNode T = CurrentNodes.Add(P, P);
                                i = T.Index;
                            }
                            CurrentNodes = CurrentNodes[i].Nodes;
                        }
                    }

                }
            }
        }

        private void btnOneRefresh_Click(object sender, EventArgs e)
        {
            string[] strings = Array.Empty<string>();
            try
            {
                strings = DllWrapper.DLLListDocs(DllWrapper.PageType.WINDOW_ONE_PAGE);
            }
            catch (InvalidOperationException)
            {
                // We assume this is cos we're not logged in
                oAuthLogonForm F = new();
                F.ShowDialog();
            }
            foreach (string s in strings)
            {
                if (s.Contains('|'))
                {
                    string[] parts = s.Split("|");
                    string[] NameParts = parts[0].Split(" - ");
                    int i = tvOne.Nodes.IndexOfKey(NameParts[0]);
                    TreeNode T;
                    if (i == -1) // Not found
                        T = tvOne.Nodes.Add(NameParts[0], NameParts[0]);
                    else
                        T = tvOne.Nodes[i];

                    TreeNode NewNode = T.Nodes.Add(NameParts[1]);
                    NewNode.Tag = parts[1];
                }
            }
        }

        private void OneNoteToRMUI_Load(object sender, EventArgs e)
        {
            DllWrapper.DLLSetLogListbox(listBox1);
            btnRMRefresh.PerformClick();
            btnOneRefresh.PerformClick();
        }

        private void btnOnePreview_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    IntPtr hDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_ONE_PAGE);
                    int Pages = DllWrapper.DLLLoadDoc(hDoc, (string)SelectedNode.Tag);
                    PreviewForm Popup = new PreviewForm();
                    Popup.hDoc = hDoc;
                    Popup.SetPages(Pages);
                    Popup.Text = SelectedNode.Text;
                    Popup.Show();
                }
        }

        private void ChkShowDebug_CheckedChanged(object sender, EventArgs e)
        {
            if (ChkShowDebug.Checked == true)
            {
                LayoutPanel.RowStyles[2].SizeType = SizeType.Absolute;
                LayoutPanel.RowStyles[2].Height = 0;
                ChkShowDebug.Text = "Shw";
            }
            else
            {
                LayoutPanel.RowStyles[2].SizeType = SizeType.Percent;
                LayoutPanel.RowStyles[2].Height = 50;
                ChkShowDebug.Text = "Hide";
            }
        }

        private void btnTimed_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode;
            DateTime RMDT = DateTime.MinValue, OneDT = DateTime.MinValue;
            IntPtr hRMDoc = 0, hOneDoc = 0;
            int RMPages = 0, ONEPages = 0;
            string RMFilename = String.Empty, ONEFilename = String.Empty;


            SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                RMFilename = SelectedNode.FullPath;
                hRMDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.TO_ONE_RM_PAGE);
                RMPages = DllWrapper.DLLLoadDoc(hRMDoc, RMFilename);
                RMDT = DllWrapper.DLLDocDateTime(hRMDoc);
                listBox1.Items.Add("RM DOC DT:" + RMDT.ToString());
            }

            SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    ONEFilename = (string)SelectedNode.Tag;
                    hOneDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.TO_RM_ONE_PAGE);
                    ONEPages = DllWrapper.DLLLoadDoc(hOneDoc, ONEFilename);
                    OneDT = DllWrapper.DLLDocDateTime(hOneDoc);
                    listBox1.Items.Add("ONE DOC DT:" + OneDT.ToString());
                }

            if (RMDT > OneDT)
            {
                listBox1.Items.Add("... RM IS Later");
                hOneDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_ONE_PAGE);
                for (int i = 0; i < RMPages; i++)
                {
                    DllWrapper.DLLConvert(hRMDoc, hOneDoc, i);
                }
                DllWrapper.DLLSaveDoc(hOneDoc, ONEFilename);
            }
            else
            {
                listBox1.Items.Add("... ONE IS Later");
                hRMDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_RM_PAGE);
                for (int i = 0; i < ONEPages; i++)
                {
                    DllWrapper.DLLConvert(hOneDoc, hRMDoc, i);
                }
                DllWrapper.DLLSaveDoc(hRMDoc, RMFilename);

            }

        }

        private void btnRM2One_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode;
            IntPtr hRMDoc = 0, hOneDoc;
            int RMPages = 0;
            string ONEFilename = String.Empty;

            SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                string RMFilename = SelectedNode.FullPath;
                hRMDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.TO_ONE_RM_PAGE);
                RMPages = DllWrapper.DLLLoadDoc(hRMDoc, RMFilename);
            }

            SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    ONEFilename = (string)SelectedNode.Tag;
                }

            hOneDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_ONE_PAGE);
            for (int i = 0; i < RMPages; i++)
            {
                DllWrapper.DLLConvert(hRMDoc, hOneDoc, i);
            }
            DllWrapper.DLLSaveDoc(hOneDoc, ONEFilename);
        }

        private void btnOne2RM_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode;
            IntPtr hRMDoc, hOneDoc = 0;
            int ONEPages = 0;
            string RMFilename = String.Empty;


            SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                RMFilename = SelectedNode.FullPath;
            }

            SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    string ONEFilename = (string)SelectedNode.Tag;
                    hOneDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.TO_RM_ONE_PAGE);
                    ONEPages = DllWrapper.DLLLoadDoc(hOneDoc, ONEFilename);
                }

            hRMDoc = DllWrapper.DLLCreateEmptyDoc(DllWrapper.PageType.WINDOW_RM_PAGE);
            for (int i = 0; i < ONEPages; i++)
            {
                DllWrapper.DLLConvert(hOneDoc, hRMDoc, i);
            }
            DllWrapper.DLLSaveDoc(hRMDoc, RMFilename);

        }
    }
}
