

/*******************************************************************************

    PreviewForm.cs

    Pop-up form for preview of RM or OneNote Doc. 
    Consists of a picture box to give me something to get a DC to pass to the 
        DLL and a slider to control the page to show. 
    Public hDoc is the document handle passed from the DLL, set by whatever 
        is loading this form

    (C) David Poirier 2026

********************************************************************************/

namespace OneNoteToRMUI
{
    public partial class PreviewForm : Form
    {
        internal DllWrapper? Doc;

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

        private void PictureBox1_Paint(object sender, PaintEventArgs e)
        {
            if (Doc is not null)
            {
                //Graphics G = pictureBox1.CreateGraphics();
                IntPtr hDC = e.Graphics.GetHdc();
                DllWrapper.Rect PageSize = Doc.DrawPage(hDC, CurrentPage);
                Size size = new(PageSize.right, PageSize.bottom);
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

        private void TrackBar1_Scroll(object sender, EventArgs e)
        {
            CurrentPage = trackBar1.Value;
            pictureBox1.Size = new(100,100); // we need to change the size, since otherwise an empty page will prevent a redraw
            pictureBox1.Invalidate();
        }
    }
}
