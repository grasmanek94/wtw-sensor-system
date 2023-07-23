namespace CoordinatorViewer
{
    partial class FormSync
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
            btn_start = new Button();
            progress_bar = new ProgressBar();
            btn_cancel = new Button();
            SuspendLayout();
            // 
            // btn_start
            // 
            btn_start.Location = new Point(12, 12);
            btn_start.Name = "btn_start";
            btn_start.Size = new Size(75, 23);
            btn_start.TabIndex = 0;
            btn_start.Text = "Start";
            btn_start.UseVisualStyleBackColor = true;
            // 
            // progress_bar
            // 
            progress_bar.Location = new Point(12, 41);
            progress_bar.Name = "progress_bar";
            progress_bar.Size = new Size(776, 23);
            progress_bar.TabIndex = 1;
            // 
            // btn_cancel
            // 
            btn_cancel.Location = new Point(93, 12);
            btn_cancel.Name = "btn_cancel";
            btn_cancel.Size = new Size(75, 23);
            btn_cancel.TabIndex = 2;
            btn_cancel.Text = "Cancel";
            btn_cancel.UseVisualStyleBackColor = true;
            // 
            // FormSync
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(800, 450);
            Controls.Add(btn_cancel);
            Controls.Add(progress_bar);
            Controls.Add(btn_start);
            Name = "FormSync";
            Text = "Sync";
            ResumeLayout(false);
        }

        #endregion

        private Button btn_start;
        private ProgressBar progress_bar;
        private Button btn_cancel;
    }
}