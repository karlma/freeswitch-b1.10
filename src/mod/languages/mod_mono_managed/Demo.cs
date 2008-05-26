﻿/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application - mod_mono
 * Copyright (C) 2008, Michael Giagnocavo <mgg@packetrino.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application - mod_mono
 *
 * The Initial Developer of the Original Code is
 * Michael Giagnocavo <mgg@packetrino.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Michael Giagnocavo <mgg@packetrino.com>
 * 
 * Demo.cs -- mod_mono demo classes
 *
 */

#if DEBUG

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace FreeSWITCH.Demo
{
    public class AppDemo : AppFunction
    {
        new protected static bool Load()
        {
            Log.WriteLine(LogLevel.Debug, "Inside AppDemo::Load.");
            return true;
        }

        new protected static void Unload()
        {
            Log.WriteLine(LogLevel.Debug, "Inside AppDemo::Unload.");
        }

        protected override void Run()
        {
            HangupFunction = hangupHook;
            Session.Answer();
            this.DtmfReceivedFunction = (d, t) => {
                Log.WriteLine(LogLevel.Info, "Received {0} for {1}.", d, t);
                return "";
            };
            Log.WriteLine(LogLevel.Debug, "Inside AppDemo.Run (args '{0}'); HookState is {1}.", Arguments, Session.HookState);
            Session.CollectDigits(5000);
            if (!IsAvailable) return; // Hungup
            Session.Hangup("USER_BUSY");
        }
        
        void hangupHook()
        {
            Log.WriteLine(LogLevel.Debug, "AppDemo hanging up, UUID: {0}.", this.Uuid);
        }
    }

    public class ApiDemo : ApiFunction
    {
        new protected static bool Load()
        {
            Log.WriteLine(LogLevel.Debug, "Inside ApiDemo::Load.");
            return true;
        }

        new protected static void Unload()
        {
            Log.WriteLine(LogLevel.Debug, "Inside ApiDemo::Unload.");
        }

        public override void ExecuteBackground(string args)
        {
            Log.WriteLine(LogLevel.Debug, "ApiDemo on a background thread #({0}), with args '{1}'.",
                System.Threading.Thread.CurrentThread.ManagedThreadId,
                args);
        }

        public override void Execute(Native.Stream stream, Native.Event evt, string args)
        {
            stream.Write(string.Format("ApiDemo executed with args '{0}' and event type {1}.",
                args, evt == null ? "<none>" : evt.GetEventType()));
        }

    }

}

#endif