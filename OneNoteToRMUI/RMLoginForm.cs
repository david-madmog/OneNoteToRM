using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OneNoteToRMUI
{
    public partial class RMLoginForm : Form
    {
        public RMLoginForm()
        {
            InitializeComponent();
        }

        private void RMLoginForm_Load(object sender, EventArgs e)
        {
            string LogonMessage = DllWrapper.GetIniSetting("RMAPI", "CodeGeneratorEndpoint");
            messageLabel.Text = LogonMessage;
            int start = LogonMessage.IndexOf("[") + 1;
            int end = LogonMessage.IndexOf("]");
            if (start > 0 && end > 0)
            {
                messageLabel.Links.Add(start, end - start, LogonMessage.Substring(start, end - start));
            }
            this.ActiveControl = codeInput;
        }

        private void messageLabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (e.Link != null)
            {
                // Determine which link was clicked within the LinkLabel.
                messageLabel.Links[messageLabel.Links.IndexOf(e.Link)].Visited = true;

                // Display the appropriate link based on the value of the 
                // LinkData property of the Link object.
                if (e.Link.LinkData != null)
                {
                    string target = e.Link.LinkData as string;
                    try
                    {
//                        Process.Start(new ProcessStartInfo(target) { UseShellExecute = true });
                        Process.Start("explorer.exe", target);
                    }
                    catch {; } // just do nothing
                }
            }
        }

        private void OKButton_Click(object sender, EventArgs e)
        {
            codeInput.TextMaskFormat = MaskFormat.ExcludePromptAndLiterals;
            string code = codeInput.Text;
            DllWrapper.DLLSetToken(DllWrapper.PageType.WINDOW_RM_PAGE, code);
            this.Close();
        }
    }
}
