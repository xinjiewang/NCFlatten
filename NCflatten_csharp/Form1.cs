using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;

namespace NCFlattenInterface
{
    public partial class Form1 : Form
    {
        static string input_parameters = "";
        static string output_parameters = System.Environment.CurrentDirectory;
        static int coordinate_parameters1 = 0;
        static int coordinate_parameters2 = 0;
        static int coordinate_parameters3 = 0;
        static int coordinate_parameters4 = 0;
        static string variable_parameters = "";
        public Form1()
        {
            InitializeComponent();
            label1.Text = output_parameters;
            folderBrowserDialog1.SelectedPath = System.Environment.CurrentDirectory;
            variable_parameters = textBox6.Text;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                textBox1.Text = "";
                input_parameters = "";
                foreach (string filename in openFileDialog1.FileNames)
                {
                    textBox1.Text += System.IO.Path.GetFullPath(filename) + Environment.NewLine;
                    input_parameters += filename + ",";
                    input_parameters.Remove(input_parameters.Length - 1);
                }
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (folderBrowserDialog1.ShowDialog() == DialogResult.OK)
            {
                label1.Text = System.IO.Path.GetFullPath(folderBrowserDialog1.SelectedPath);
                output_parameters = label1.Text;
            }
        }

        private void textBox2_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar))
            {
                e.Handled = true;
            }
        }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
            coordinate_parameters1 = Int32.Parse(textBox2.Text);
        }

        private void textBox3_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar))
            {
                e.Handled = true;
            }
        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {
            coordinate_parameters2 = Int32.Parse(textBox3.Text);
        }

        private void textBox4_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar))
            {
                e.Handled = true;
            }
        }

        private void textBox4_TextChanged(object sender, EventArgs e)
        {
            coordinate_parameters3 = Int32.Parse(textBox4.Text);
        }

        private void textBox5_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (!char.IsControl(e.KeyChar) && !char.IsDigit(e.KeyChar))
            {
                e.Handled = true;
            }
        }
        private void textBox5_TextChanged(object sender, EventArgs e)
        {
            coordinate_parameters4 = Int32.Parse(textBox5.Text);
        }


        private void textBox6_TextChanged(object sender, EventArgs e)
        {
            variable_parameters = textBox6.Text;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (input_parameters == "")
            {
                MessageBox.Show("请先选择NC文件（可以一次选择多个）", "提示", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }


            string coordinate_parameters = "";
            int if_continue = 6;
            if (coordinate_parameters3 < coordinate_parameters1) coordinate_parameters3 = coordinate_parameters1;
            if (coordinate_parameters4 < coordinate_parameters2) coordinate_parameters4 = coordinate_parameters2;

            if (coordinate_parameters1 + coordinate_parameters2 + coordinate_parameters3 + coordinate_parameters4 > 0)
            {
                coordinate_parameters = coordinate_parameters1.ToString() + "," + coordinate_parameters2.ToString() + "," + coordinate_parameters3.ToString() + "," + coordinate_parameters4.ToString();

                int coordinate_count = (coordinate_parameters3 - coordinate_parameters1 + 1) * (coordinate_parameters4 - coordinate_parameters2 + 1); 
                if (coordinate_count > 100)
                    if_continue = (int)MessageBox.Show("您指定了" + coordinate_count + "个网格点，这将生成" + coordinate_count + "个文本文件，用时较慢，确定生成？", "提示", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                else
                    if_continue = (int)MessageBox.Show("您指定了" + coordinate_count + "个网格点，这将生成" + coordinate_count + "个文本文件，确定生成？", "提示", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            }
            else
            {
                if_continue = (int)MessageBox.Show("您指定了所有网格点，这将生成大量文本文件，用时较慢，确定生成？", "提示", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            }

            if (if_continue == 6)
            {
                string final_parameter = "-i " + input_parameters + " -o " + output_parameters + " -v " + variable_parameters;
                if (coordinate_parameters != "") final_parameter += " -c " + coordinate_parameters;


                try
                {
                    // Start the process with the info we specified.
                    // Call WaitForExit and then the using statement will close.
                    using (Process exeProcess = System.Diagnostics.Process.Start("CMD.exe", "/C " + System.Environment.CurrentDirectory + "\\NCFlatten.exe " + final_parameter))
                    {
                        exeProcess.WaitForExit();
                    }
                }
                catch (Exception ex)
                {
                    // Log error.
                    MessageBox.Show("发生了错误，错误提示为：" + ex, "提示", MessageBoxButtons.OK, MessageBoxIcon.Information);

                }
            }

        }
    }
}
