/*
 * opencog/embodiment/Control/Procedure/ComboPAIExecutable.cc
 *
 * Copyright (C) 2007-2008 TO_COMPLETE
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "ComboShellServer.h"
#include "util/files.h"
#include <EmbodimentConfig.h>

using namespace opencog;
using namespace MessagingSystem;

int main(int argc, char** argv)
{

    // set up the system for talking to the router
    config(Control::EmbodimentConfig::embodimentCreateInstance, true);

    // if exists load file with configuration parameters
    // IMPORTANT: this file should be the same for all executables that create
    // a EmbodimentConfig object.

    if (fileExists(config().get("CONFIG_FILE").c_str())) {
        config().load(config().get("CONFIG_FILE").c_str());
    }

    server(ComboShellServer::createInstance);
    ComboShellServer& css = static_cast<ComboShellServer&>(server());
    css.init();
    css.serverLoop();
    return 0;
}
