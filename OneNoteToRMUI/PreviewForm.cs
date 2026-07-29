using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace OneNoteToRMUI
{
    public partial class PreviewForm : Form
    {
        public IntPtr hDoc = IntPtr.Zero;

        private int Pages = 0;

        private int CurrentPage = 0;

        public void SetPages(int value)
        { 
            trackBar1.Maximum = value-1; 
            Pages = value-1;
        } 

        public PreviewForm()
        {
            InitializeComponent();
        }

        private void pictureBox1_Paint(object sender, PaintEventArgs e)
        {
            if (hDoc != IntPtr.Zero)
            {
                //Graphics G = pictureBox1.CreateGraphics();
                IntPtr hDC = e.Graphics.GetHdc();
                DllWrapper.Rect PageSize = DllWrapper.DLLDrawPage(hDoc, hDC, CurrentPage);
                Size size = new Size(PageSize.right, PageSize.bottom);
                pictureBox1.Size = size;
                e.Graphics.ReleaseHdc(hDC);
            }
        }

        private void PopupForm_Load(object sender, EventArgs e)
        {

        }

        private void PopupForm_Shown(object sender, EventArgs e)
        {
            pictureBox1.Invalidate();
        }

        private void trackBar1_Scroll(object sender, EventArgs e)
        {
            CurrentPage = trackBar1.Value;
            pictureBox1.Invalidate();
        }
    }
}
