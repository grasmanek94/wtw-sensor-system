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
            plots_panel = new TableLayoutPanel();
            split_container = new SplitContainer();
            ((System.ComponentModel.ISupportInitialize)data_grid).BeginInit();
            ((System.ComponentModel.ISupportInitialize)split_container).BeginInit();
            split_container.Panel1.SuspendLayout();
            split_container.Panel2.SuspendLayout();
            split_container.SuspendLayout();
            SuspendLayout();
            // 
            // data_grid
            // 
            data_grid.AllowUserToAddRows = false;
            data_grid.AllowUserToDeleteRows = false;
            data_grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
            data_grid.AutoSizeRowsMode = DataGridViewAutoSizeRowsMode.DisplayedCells;
            data_grid.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            data_grid.Dock = DockStyle.Fill;
            data_grid.Location = new Point(0, 0);
            data_grid.Name = "data_grid";
            data_grid.ReadOnly = true;
            data_grid.RowTemplate.Height = 25;
            data_grid.Size = new Size(1064, 134);
            data_grid.TabIndex = 0;
            // 
            // plots_panel
            // 
            plots_panel.AutoSize = true;
            plots_panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            plots_panel.ColumnCount = 2;
            plots_panel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            plots_panel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
            plots_panel.Dock = DockStyle.Fill;
            plots_panel.Location = new Point(0, 0);
            plots_panel.Name = "plots_panel";
            plots_panel.RowCount = 2;
            plots_panel.RowStyles.Add(new RowStyle(SizeType.Percent, 50F));
            plots_panel.RowStyles.Add(new RowStyle(SizeType.Percent, 50F));
            plots_panel.Size = new Size(1064, 566);
            plots_panel.TabIndex = 1;
            // 
            // split_container
            // 
            split_container.Dock = DockStyle.Fill;
            split_container.Location = new Point(0, 0);
            split_container.Name = "split_container";
            split_container.Orientation = Orientation.Horizontal;
            // 
            // split_container.Panel1
            // 
            split_container.Panel1.Controls.Add(data_grid);
            split_container.Panel1MinSize = 125;
            // 
            // split_container.Panel2
            // 
            split_container.Panel2.Controls.Add(plots_panel);
            split_container.Size = new Size(1064, 704);
            split_container.SplitterDistance = 134;
            split_container.TabIndex = 2;
            // 
            // FormAllDevicesViewer
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1064, 704);
            Controls.Add(split_container);
            Name = "FormAllDevicesViewer";
            Text = "All Devices Viewer";
            ((System.ComponentModel.ISupportInitialize)data_grid).EndInit();
            split_container.Panel1.ResumeLayout(false);
            split_container.Panel2.ResumeLayout(false);
            split_container.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)split_container).EndInit();
            split_container.ResumeLayout(false);
            ResumeLayout(false);
        }

        #endregion

        private DataGridView data_grid;
        private TableLayoutPanel plots_panel;
        private SplitContainer split_container;
    }
}