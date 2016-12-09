﻿//-----------------------------------------------------------------------
// <copyright file="App.xaml.cs" company="AllSeen Alliance.">
//     Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
//        Source Project (AJOSP) Contributors and others.
//
//        SPDX-License-Identifier: Apache-2.0
//
//        All rights reserved. This program and the accompanying materials are
//        made available under the terms of the Apache License, Version 2.0
//        which accompanies this distribution, and is available at
//        http://www.apache.org/licenses/LICENSE-2.0
//
//        Copyright 2016 Open Connectivity Foundation and Contributors to
//        AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for
//        any purpose with or without fee is hereby granted, provided that the
//        above copyright notice and this permission notice appear in all
//        copies.
//
//         THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//         WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//         WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//         AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//         DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//         PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//         TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//         PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

namespace Sessions
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using Windows.ApplicationModel;
    using Windows.ApplicationModel.Activation;
    using Windows.Foundation;
    using Windows.Foundation.Collections;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;
    using Windows.UI.Xaml.Controls.Primitives;
    using Windows.UI.Xaml.Data;
    using Windows.UI.Xaml.Input;
    using Windows.UI.Xaml.Media;
    using Windows.UI.Xaml.Navigation;

    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    public sealed partial class App : Application
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="App"/> class which is the singleton 
        /// application object.  This is the first line of authored code executed, and as such 
        /// is the logical equivalent of main() or WinMain().</summary>
        public App()
        {
            this.InitializeComponent();
            this.Suspending += this.OnSuspending;
        }

        /// <summary>
        /// Invoked when the application is launched normally by the end user.  Other entry points
        /// will be used when the application is launched to open a specific file, to display
        /// search results, and so forth.
        /// </summary>
        /// <param name="args">Details about the launch request and process.</param>
        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            Frame rootFrame = Window.Current.Content as Frame;

            // Do not repeat app initialization when the Window already has content,
            // just ensure that the window is active
            if (rootFrame == null)
            {
                // Create a Frame to act as the navigation context and navigate to the first page
                rootFrame = new Frame();

                if (args.PreviousExecutionState == ApplicationExecutionState.Terminated)
                {
                    //// TODO: Load state from previously suspended application
                }

                // Place the frame in the current Window
                Window.Current.Content = rootFrame;
            }

            if (rootFrame.Content == null)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                if (!rootFrame.Navigate(typeof(MainPage), args.Arguments))
                {
                    throw new Exception("Failed to create initial page");
                }
            }

            // Ensure the current window is active
            Window.Current.Activate();
        }

        /// <summary>
        /// Invoked when application execution is being suspended.  Application state is saved
        /// without knowing whether the application will be terminated or resumed with the contents
        /// of memory still intact.
        /// </summary>
        /// <param name="sender">The source of the suspend request.</param>
        /// <param name="e">Details about the suspend request.</param>
        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            //// TODO: Save application state and stop any background activity
            deferral.Complete();
        }
    }
}