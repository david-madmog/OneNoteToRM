
namespace OneNoteToRMUI
{
    partial class OneNoteToRMUI
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(OneNoteToRMUI));
            LayoutPanel = new TableLayoutPanel();
            panel1 = new Panel();
            btnRMRefresh = new Button();
            btnRMPreview = new Button();
            panel2 = new Panel();
            btnOnePreview = new Button();
            btnOneRefresh = new Button();
            listBox1 = new ListBox();
            tvRM = new TreeView();
            tvOne = new TreeView();
            ChkShowDebug = new CheckBox();
            panel3 = new Panel();
            btnOne2RM = new Button();
            btnTimed = new Button();
            btnRM2One = new Button();
            LayoutPanel.SuspendLayout();
            panel1.SuspendLayout();
            panel2.SuspendLayout();
            panel3.SuspendLayout();
            SuspendLayout();
            // 
            // LayoutPanel
            // 
            LayoutPanel.ColumnCount = 3;
            LayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            LayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 65F));
            LayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            LayoutPanel.Controls.Add(panel1, 0, 1);
            LayoutPanel.Controls.Add(panel2, 2, 1);
            LayoutPanel.Controls.Add(listBox1, 0, 2);
            LayoutPanel.Controls.Add(tvRM, 0, 0);
            LayoutPanel.Controls.Add(tvOne, 2, 0);
            LayoutPanel.Controls.Add(ChkShowDebug, 1, 1);
            LayoutPanel.Controls.Add(panel3, 1, 0);
            LayoutPanel.Dock = DockStyle.Fill;
            LayoutPanel.Location = new Point(0, 0);
            LayoutPanel.Name = "LayoutPanel";
            LayoutPanel.RowCount = 3;
            LayoutPanel.RowStyles.Add(new RowStyle(SizeType.Percent, 64.3086853F));
            LayoutPanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 43F));
            LayoutPanel.RowStyles.Add(new RowStyle(SizeType.Percent, 35.69131F));
            LayoutPanel.Size = new Size(768, 413);
            LayoutPanel.TabIndex = 5;
            // 
            // panel1
            // 
            panel1.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            panel1.Controls.Add(btnRMRefresh);
            panel1.Controls.Add(btnRMPreview);
            panel1.Location = new Point(0, 237);
            panel1.Margin = new Padding(0);
            panel1.Name = "panel1";
            panel1.Size = new Size(351, 43);
            panel1.TabIndex = 10;
            // 
            // btnRMRefresh
            // 
            btnRMRefresh.Location = new Point(0, 0);
            btnRMRefresh.Name = "btnRMRefresh";
            btnRMRefresh.Size = new Size(140, 40);
            btnRMRefresh.TabIndex = 11;
            btnRMRefresh.Text = "Refresh";
            btnRMRefresh.UseVisualStyleBackColor = true;
            btnRMRefresh.Click += btnRMRefresh_Click;
            // 
            // btnRMPreview
            // 
            btnRMPreview.Location = new Point(146, 0);
            btnRMPreview.Name = "btnRMPreview";
            btnRMPreview.Size = new Size(140, 40);
            btnRMPreview.TabIndex = 10;
            btnRMPreview.Text = "Preview";
            btnRMPreview.UseVisualStyleBackColor = true;
            btnRMPreview.Click += btnRMPreview_Click;
            // 
            // panel2
            // 
            panel2.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            panel2.Controls.Add(btnOnePreview);
            panel2.Controls.Add(btnOneRefresh);
            panel2.Location = new Point(416, 237);
            panel2.Margin = new Padding(0);
            panel2.Name = "panel2";
            panel2.Size = new Size(352, 43);
            panel2.TabIndex = 11;
            // 
            // btnOnePreview
            // 
            btnOnePreview.Location = new Point(146, 0);
            btnOnePreview.Name = "btnOnePreview";
            btnOnePreview.Size = new Size(140, 40);
            btnOnePreview.TabIndex = 10;
            btnOnePreview.Text = "Preview";
            btnOnePreview.UseVisualStyleBackColor = true;
            btnOnePreview.Click += btnOnePreview_Click;
            // 
            // btnOneRefresh
            // 
            btnOneRefresh.Location = new Point(0, 0);
            btnOneRefresh.Name = "btnOneRefresh";
            btnOneRefresh.Size = new Size(140, 40);
            btnOneRefresh.TabIndex = 9;
            btnOneRefresh.Text = "Refresh";
            btnOneRefresh.UseVisualStyleBackColor = true;
            btnOneRefresh.Click += btnOneRefresh_Click;
            // 
            // listBox1
            // 
            LayoutPanel.SetColumnSpan(listBox1, 3);
            listBox1.Dock = DockStyle.Fill;
            listBox1.FormattingEnabled = true;
            listBox1.Location = new Point(3, 283);
            listBox1.Name = "listBox1";
            listBox1.Size = new Size(762, 127);
            listBox1.TabIndex = 12;
            // 
            // tvRM
            // 
            tvRM.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            tvRM.HideSelection = false;
            tvRM.Location = new Point(3, 3);
            tvRM.Name = "tvRM";
            tvRM.Size = new Size(345, 231);
            tvRM.TabIndex = 13;
            // 
            // tvOne
            // 
            tvOne.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            tvOne.HideSelection = false;
            tvOne.Location = new Point(419, 3);
            tvOne.Name = "tvOne";
            tvOne.Size = new Size(346, 231);
            tvOne.TabIndex = 14;
            // 
            // ChkShowDebug
            // 
            ChkShowDebug.Appearance = Appearance.Button;
            ChkShowDebug.AutoSize = true;
            ChkShowDebug.Location = new Point(354, 240);
            ChkShowDebug.Name = "ChkShowDebug";
            ChkShowDebug.Size = new Size(51, 30);
            ChkShowDebug.TabIndex = 15;
            ChkShowDebug.Text = "Hide";
            ChkShowDebug.UseVisualStyleBackColor = true;
            ChkShowDebug.CheckedChanged += ChkShowDebug_CheckedChanged;
            // 
            // panel3
            // 
            panel3.Controls.Add(btnOne2RM);
            panel3.Controls.Add(btnTimed);
            panel3.Controls.Add(btnRM2One);
            panel3.Dock = DockStyle.Fill;
            panel3.Location = new Point(354, 3);
            panel3.Name = "panel3";
            panel3.Size = new Size(59, 231);
            panel3.TabIndex = 16;
            // 
            // btnOne2RM
            // 
            btnOne2RM.Font = new Font("Segoe UI", 10.2F);
            btnOne2RM.Location = new Point(0, 137);
            btnOne2RM.Name = "btnOne2RM";
            btnOne2RM.Size = new Size(59, 32);
            btnOne2RM.TabIndex = 2;
            btnOne2RM.Text = "←";
            btnOne2RM.UseVisualStyleBackColor = true;
            btnOne2RM.Click += btnOne2RM_Click;
            // 
            // btnTimed
            // 
            btnTimed.Font = new Font("Segoe UI", 10.2F);
            btnTimed.Location = new Point(0, 99);
            btnTimed.Name = "btnTimed";
            btnTimed.Size = new Size(59, 32);
            btnTimed.TabIndex = 1;
            btnTimed.Text = "←?→";
            btnTimed.UseVisualStyleBackColor = true;
            btnTimed.Click += btnTimed_Click;
            // 
            // btnRM2One
            // 
            btnRM2One.Font = new Font("Segoe UI", 10.2F);
            btnRM2One.Location = new Point(0, 61);
            btnRM2One.Name = "btnRM2One";
            btnRM2One.Size = new Size(59, 32);
            btnRM2One.TabIndex = 0;
            btnRM2One.Text = "→";
            btnRM2One.UseVisualStyleBackColor = true;
            btnRM2One.Click += btnRM2One_Click;
            // 
            // OneNoteToRMUI
            // 
            AutoScaleDimensions = new SizeF(8F, 20F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(768, 413);
            Controls.Add(LayoutPanel);
            Icon = (Icon)resources.GetObject("$this.Icon");
            Name = "OneNoteToRMUI";
            Text = "One Note to RM";
            Load += OneNoteToRMUI_Load;
            LayoutPanel.ResumeLayout(false);
            LayoutPanel.PerformLayout();
            panel1.ResumeLayout(false);
            panel2.ResumeLayout(false);
            panel3.ResumeLayout(false);
            ResumeLayout(false);
        }

        #endregion

        private TableLayoutPanel LayoutPanel;
        private Panel panel1;
        private Button btnRMRefresh;
        private Button btnRMPreview;
        private Panel panel2;
        private Button btnOnePreview;
        private Button btnOneRefresh;
        private ListBox listBox1;
        private TreeView tvRM;
        private TreeView tvOne;
        private CheckBox ChkShowDebug;
        private Panel panel3;
        private Button btnOne2RM;
        private Button btnTimed;
        private Button btnRM2One;
    }
}
