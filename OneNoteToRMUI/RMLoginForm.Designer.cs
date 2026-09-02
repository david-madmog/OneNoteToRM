namespace OneNoteToRMUI
{
    partial class RMLoginForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
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
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            OKButton = new Button();
            codeInput = new MaskedTextBox();
            cancelButton = new Button();
            messageLabel = new LinkLabel();
            SuspendLayout();
            // 
            // OKButton
            // 
            OKButton.DialogResult = DialogResult.OK;
            OKButton.Location = new Point(12, 314);
            OKButton.Name = "OKButton";
            OKButton.Size = new Size(137, 37);
            OKButton.TabIndex = 0;
            OKButton.Text = "OK";
            OKButton.UseVisualStyleBackColor = true;
            OKButton.Click += OKButton_Click;
            // 
            // codeInput
            // 
            codeInput.AllowPromptAsInput = false;
            codeInput.AsciiOnly = true;
            codeInput.CutCopyMaskFormat = MaskFormat.ExcludePromptAndLiterals;
            codeInput.Font = new Font("Segoe UI", 48F, FontStyle.Regular, GraphicsUnit.Point, 0);
            codeInput.Location = new Point(12, 138);
            codeInput.Mask = "<L L L L L L L L";
            codeInput.Name = "codeInput";
            codeInput.RejectInputOnFirstFailure = true;
            codeInput.Size = new Size(527, 114);
            codeInput.TabIndex = 2;
            codeInput.TextAlign = HorizontalAlignment.Center;
            codeInput.TextMaskFormat = MaskFormat.ExcludePromptAndLiterals;
            // 
            // cancelButton
            // 
            cancelButton.DialogResult = DialogResult.Cancel;
            cancelButton.Location = new Point(155, 314);
            cancelButton.Name = "cancelButton";
            cancelButton.Size = new Size(137, 37);
            cancelButton.TabIndex = 3;
            cancelButton.Text = "Cancel";
            cancelButton.UseVisualStyleBackColor = true;
            // 
            // messageLabel
            // 
            messageLabel.Location = new Point(12, 9);
            messageLabel.Name = "messageLabel";
            messageLabel.Size = new Size(525, 126);
            messageLabel.TabIndex = 4;
            messageLabel.TabStop = true;
            messageLabel.Text = "...";
            messageLabel.TextAlign = ContentAlignment.MiddleCenter;
            messageLabel.LinkClicked += MessageLabel_LinkClicked;
            // 
            // RMLoginForm
            // 
            AcceptButton = OKButton;
            AutoScaleDimensions = new SizeF(8F, 20F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(549, 363);
            Controls.Add(messageLabel);
            Controls.Add(cancelButton);
            Controls.Add(codeInput);
            Controls.Add(OKButton);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MdiChildrenMinimizedAnchorBottom = false;
            MinimizeBox = false;
            Name = "RMLoginForm";
            Text = "Login";
            Load += RMLoginForm_Load;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button OKButton;
        private MaskedTextBox codeInput;
        private Button cancelButton;
        private LinkLabel messageLabel;
    }
}