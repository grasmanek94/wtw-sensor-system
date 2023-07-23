namespace CoordinatorViewer
{
    partial class FormAllDevicesViewer
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
            data_grid = new DataGridView();
            ((System.ComponentModel.ISupportInitialize)data_grid).BeginInit();
            SuspendLayout();
            // 
            // data_grid
            // 
            data_grid.AllowUserToAddRows = false;
            data_grid.AllowUserToDeleteRows = false;
            data_grid.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            data_grid.Dock = DockStyle.Fill;
            data_grid.Location = new Point(0, 0);
            data_grid.Name = "data_grid";
            data_grid.ReadOnly = true;
            data_grid.RowTemplate.Height = 25;
            data_grid.Size = new Size(1002, 545);
            data_grid.TabIndex = 0;
            // 
            // FormAllDevicesViewer
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1002, 545);
            Controls.Add(data_grid);
            Name = "FormAllDevicesViewer";
            Text = "All Devices Viewer";
            ((System.ComponentModel.ISupportInitialize)data_grid).EndInit();
            ResumeLayout(false);
        }

        #endregion

        private DataGridView data_grid;
    }
}