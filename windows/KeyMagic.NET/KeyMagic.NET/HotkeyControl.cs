﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace KeyMagic
{
    public partial class HotkeyControl : UserControl
    {
        public event EventHandler ValueChanged;

        private void OnValueChanged()
        {
            if (ValueChanged != null) ValueChanged(this, EventArgs.Empty);
        }

        public HotkeyControl()
        {
            InitializeComponent();
        }

        public HotkeyControl(Hotkey hk)
        {
            InitializeComponent();

            chkAlt.Checked = hk.alt;
            chkCtrl.Checked = hk.ctrl;
            chkShift.Checked = hk.shift;
            cboKey.Text = hk.keyString;
        }

        public Hotkey Hotkey
        {
            get
            {
                return new Hotkey(chkCtrl.Checked, chkAlt.Checked, chkShift.Checked, cboKey.Text);
            }
            set
            {
                chkAlt.Checked = value.alt;
                chkCtrl.Checked = value.ctrl;
                chkShift.Checked = value.shift;
                cboKey.Text = value.keyString;
            }
        }

        private void cboKey_SelectedIndexChanged(object sender, EventArgs e)
        {
            OnValueChanged();
        }

        private void chkControl_CheckedChanged(object sender, EventArgs e)
        {
            OnValueChanged();
        }
    }
}