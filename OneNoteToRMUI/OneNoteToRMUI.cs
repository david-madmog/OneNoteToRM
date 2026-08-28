/*******************************************************************************

    OneNoteToRMUI.cs

    Main code for UI

    (C) David Poirier 2026

********************************************************************************/

using Microsoft.VisualBasic;
using System.Security.Policy;

namespace OneNoteToRMUI
{
    public partial class OneNoteToRMUI : Form
    {
        public OneNoteToRMUI()
        {
            InitializeComponent();
        }

        // In Wingdings, these are clock icons to provide timer animation
        private readonly string TimerCounting = "·¸¹º»¼½¾¿ÀÁÂ";
        private int TimerCount = 999;

        private void BtnRMPreview_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                if (SelectedNode.Tag != null)
                {
                    //                string FileName = SelectedNode.FullPath;
                    string FileName = (string)SelectedNode.Tag;
                    DllWrapper Doc = new(DllWrapper.PageType.WINDOW_RM_PAGE);
                    int Pages = Doc.Load(FileName);
                    PreviewForm Popup = new()
                    {
                        Doc = Doc
                    };
                    Popup.SetPages(Pages);
                    Popup.Text = FileName;
                    Popup.Show();
                }
            }
        }

        private void BtnRMRefresh_Click(object sender, EventArgs e)
        {
            Task Tsk = Task.Run(() => {
                string[] strings = [];
                try
                {
                    strings = DllWrapper.DLLListDocs(DllWrapper.PageType.WINDOW_RM_PAGE);
                }
                catch (InvalidOperationException)
                {
                    // We assume this is cos we're not logged in
                    RMLoginForm F = new();
                    F.ShowDialog();
                }
                ParseDocList(strings, tvRM);
            });
        }

        private void BtnOneRefresh_Click(object sender, EventArgs e)
        {
            Task Tsk = Task.Run(() =>
            {
                string[] strings = [];
                try
                {
                    strings = DllWrapper.DLLListDocs(DllWrapper.PageType.WINDOW_ONE_PAGE);
                }
                catch (InvalidOperationException)
                {
                    // We assume this is cos we're not logged in
                    OAuthLogonForm F = new();
                    F.ShowDialog();
                }
                ParseDocList(strings, tvOne);
            });
        }

        private void OneNoteToRMUI_Load(object sender, EventArgs e)
        {
            DllWrapper.DLLSetLogListbox(listBox1);
            ChkShowDebug.Checked = false;
            btnRMRefresh.PerformClick();
            btnOneRefresh.PerformClick();
        }

        private void BtnOnePreview_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    DllWrapper Doc = new(DllWrapper.PageType.WINDOW_ONE_PAGE);
                    int Pages = Doc.Load((string)SelectedNode.Tag);
                    PreviewForm Popup = new()
                    {
                        Doc = Doc
                    };
                    Popup.SetPages(Pages);
                    Popup.Text = SelectedNode.Text;
                    Popup.Show();
                }
        }

        private void ChkShowDebug_CheckedChanged(object sender, EventArgs e)
        {
            if (ChkShowDebug.Checked == true)
            {
                LayoutPanel.RowStyles[2].SizeType = SizeType.Percent;
                LayoutPanel.RowStyles[2].Height = 50;
                ChkShowDebug.Text = "Hide Debug";
            }
            else
            {
                LayoutPanel.RowStyles[2].SizeType = SizeType.Absolute;
                LayoutPanel.RowStyles[2].Height = 0;
                ChkShowDebug.Text = "";
            }
        }

        private void BtnTimed_Click(object sender, EventArgs e)
        {
            if (tvRM.SelectedNode != null && tvOne.SelectedNode != null)
                Program.DoCheck(tvRM.SelectedNode.Text, tvOne.SelectedNode.Text);
        }

        private void BtnRM2One_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode;
            DllWrapper? RMDoc = null;
            int RMPages = 0;
            string ONEFilename = String.Empty;

            SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
            {
                string RMFilename = SelectedNode.FullPath;
                RMDoc = new(DllWrapper.PageType.TO_ONE_RM_PAGE);
                RMPages = RMDoc.Load(RMFilename);
            }

            SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    ONEFilename = (string)SelectedNode.Tag;
                }

            DllWrapper? OneDoc = new(DllWrapper.PageType.WINDOW_ONE_PAGE);

            if (RMDoc is null || OneDoc is null)
                return;

            for (int i = 0; i < RMPages; i++)
            {
                RMDoc.Convert(OneDoc, i);
            }
            OneDoc.Save(ONEFilename);
        }

        private void BtnOne2RM_Click(object sender, EventArgs e)
        {
            TreeNode? SelectedNode;
            DllWrapper? OneDoc = null;
            int ONEPages = 0;
            string RMFilename = String.Empty;


            SelectedNode = tvRM.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    RMFilename = (string)SelectedNode.Tag;
                }

            SelectedNode = tvOne.SelectedNode;
            if (SelectedNode != null)
                if (SelectedNode.Tag != null)
                {
                    string ONEFilename = (string)SelectedNode.Tag;
                    OneDoc = new(DllWrapper.PageType.TO_RM_ONE_PAGE);
                    ONEPages = OneDoc.Load(ONEFilename);
                }

            DllWrapper? RMDoc = new(DllWrapper.PageType.WINDOW_RM_PAGE);

            if (RMDoc is null || OneDoc is null)
                return;

            for (int i = 0; i < ONEPages; i++)
            {
                OneDoc.Convert(RMDoc, i);
            }
            RMDoc.Save(RMFilename);

        }

        private void ChkAuto_CheckedChanged(object sender, EventArgs e)
        {
            if (chkAuto.Checked)
            {
                AutoTimer.Enabled = true;
                btnOne2RM.Enabled = false;
                btnRM2One.Enabled = false;
                btnTimed.Enabled = false;

            }
            else
            {
                AutoTimer.Enabled = false;
                btnOne2RM.Enabled = true;
                btnRM2One.Enabled = true;
                btnTimed.Enabled = true;
            }
        }

        private void AutoTimer_Tick(object sender, EventArgs e)
        {
            if (++TimerCount >= TimerCounting.Length)
            {
                if (tvRM.SelectedNode != null && tvOne.SelectedNode != null)
                    Program.DoCheck(tvRM.SelectedNode.Text, tvOne.SelectedNode.Text);

                TimerCount = 0;

            }
            chkAuto.Text = string.Concat(btnOne2RM.Text, TimerCounting.AsSpan(TimerCount, 1), btnRM2One.Text);
        }

        //public void DoCheck(DateTime LastCheck)
        //{
        //    TreeNode? SelectedNode;
        //    DateTime RMDT = DateTime.MinValue, OneDT = DateTime.MinValue;
        //    DllWrapper? RMDoc = null, OneDoc = null;
        //    int RMPages = 0, ONEPages = 0;
        //    string RMFilename = String.Empty, ONEFilename = String.Empty;


        //    SelectedNode = tvRM.SelectedNode;
        //    if (SelectedNode != null)
        //    {
        //        RMFilename = SelectedNode.FullPath;
        //        RMDoc = new(DllWrapper.PageType.TO_ONE_RM_PAGE);
        //        RMPages = RMDoc.Load(RMFilename);
        //        RMDT = RMDoc.DocDateTime();
        //        listBox1.Items.Add("RM DOC DT:" + RMDT.ToString());
        //    }

        //    SelectedNode = tvOne.SelectedNode;
        //    if (SelectedNode != null)
        //        if (SelectedNode.Tag != null)
        //        {
        //            ONEFilename = (string)SelectedNode.Tag;
        //            OneDoc = new(DllWrapper.PageType.TO_RM_ONE_PAGE);
        //            ONEPages = OneDoc.Load(ONEFilename);
        //            OneDT = OneDoc.DocDateTime();
        //            listBox1.Items.Add("ONE DOC DT:" + OneDT.ToString());
        //        }

        //    if (RMDT > OneDT)
        //    {
        //        listBox1.Items.Add("... RM is Later");
        //        if (RMDT > LastCheck)
        //        {
        //            OneDoc = new(DllWrapper.PageType.WINDOW_ONE_PAGE);
        //            for (int i = 0; i < RMPages; i++)
        //            {
        //                RMDoc?.Convert(OneDoc, i);
        //            }
        //            OneDoc.Save(ONEFilename);
        //        }else
        //        {
        //            listBox1.Items.Add("... but not changed");
        //        }
        //    }
        //    else
        //    {
        //        listBox1.Items.Add("... ONE is Later");
        //        if (OneDT > LastCheck)
        //        {
        //            RMDoc = new(DllWrapper.PageType.WINDOW_RM_PAGE);
        //            for (int i = 0; i < ONEPages; i++)
        //            {
        //                OneDoc?.Convert(RMDoc, i);
        //            }
        //            RMDoc.Save(RMFilename);
        //        }
        //        else
        //        {
        //            listBox1.Items.Add("... but not changed");
        //        }

        //    }

        //}

        private static void ParseDocList(string[] strings, TreeView tv)
        {
            tv.Invoke(() =>
            {
                string[] parts;
                string[] NameParts;
                tv.Nodes.Clear();
                foreach (string s in strings)
                {
                    if (s.Contains('|'))
                    {
                        parts = s.Split("|");
                    }
                    else
                    {
                        parts = [s, ""]; // ensure there's always 2 parts
                    }

                    NameParts = parts[0].Split(tv.PathSeparator);
                    if (NameParts.Length == 1)
                    {
                        if (parts[0].Length > 0)
                        {
                            TreeNode T = tv.Nodes.Add(parts[0]);
                            T.Tag = parts[1];
                        }         
                    }
                    else
                    {
                        TreeNodeCollection CurrentNodes = tv.Nodes;
                        foreach (string P in NameParts)
                        {
                            int i = CurrentNodes.IndexOfKey(P);
                            if (i == -1) // Not found
                            {
                                TreeNode T = CurrentNodes.Add(P, P);
                                T.Tag = parts[1];
                                i = T.Index;
                            }
                            CurrentNodes = CurrentNodes[i].Nodes;
                        }
                    }

                }
            });
        }
    }
}
