# # Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
#    Source Project (AJOSP) Contributors and others.
#
#    SPDX-License-Identifier: Apache-2.0
#
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
#    Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for
#    any purpose with or without fee is hereby granted, provided that the
#    above copyright notice and this permission notice appear in all
#    copies.
#
#     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
#     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
#     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
#     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#     PERFORMANCE OF THIS SOFTWARE.
# 

import os

# Get the global environment
env = SConscript(['build_core/SConscript'])

vars = Variables()

vars.Add('BINDINGS', 'Bindings to build (comma separated list): cpp, c, java, js, unity', 'cpp,c,java,js,unity')
vars.Add('SERVICES', 'AllJoyn services libraries to build (comma separated list): about, config, controlpanel, notification, onboarding, audio', 'about')
vars.Add(EnumVariable('BUILD_SERVICES_SAMPLES', 'Build the services samples that require libxml2 and json libraries.', 'on', allowed_values = ['on', 'off']))
vars.Update(env)
Help(vars.GenerateHelpText(env))

bindings = set([ b.strip()
                 for b in env['BINDINGS'].split(',')
                 if b.strip() == 'cpp' or os.path.exists('alljoyn_%s/SConscript' % b.strip()) ])
services = set([ s.strip()
                 for s in env['SERVICES'].split(',')
                 if os.path.exists('services/%s/SConscript' % s.strip())])

print 'Building bindings: %s' % ', '.join(bindings)
print 'Building services: %s' % ', '.join(services)

env['bindings'] = bindings
env['services'] = services

# Always build AllJoyn Core
env.SConscript(['alljoyn_core/SConscript'])

if bindings.intersection(['c', 'unity']):
    # unity depends on the C bindings
    env.SConscript(['alljoyn_c/SConscript'])

    if 'unity' in bindings:
        env.SConscript(['alljoyn_unity/SConscript'])

if 'java' in bindings:
    env.SConscript(['alljoyn_java/SConscript'])

if 'js' in bindings:
    env.SConscript(['alljoyn_js/SConscript'])


if services.intersection(['about', 'config', 'controlpanel', 'notification', 'onboarding', 'audio']):
    # config, controlpanel, notification, and onboarding all depend on about.
    env.SConscript(['services/about/SConscript'])

    if services.intersection(['config', 'onboarding']):
        # onboarding also depends on config
        env.SConscript(['services/config/SConscript'])

        if 'onboarding' in services:
            env.SConscript(['services/onboarding/SConscript'])

    if 'controlpanel' in services:
        env.SConscript(['services/controlpanel/SConscript'])

    if 'notification' in services:
        env.SConscript(['services/notification/SConscript'])

    if 'audio' in services:
        env.SConscript(['services/audio/SConscript'])


#Build Win7 SDK installer
if env.has_key('WIN7_MSI') and env['WIN7_MSI'] == 'true':
    win7Sdk = env.SConscript(['alljoyn_core/install/Win7/SConscript'])
    env.Depends(win7Sdk, installedFiles)