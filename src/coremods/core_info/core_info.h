/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2018, 2020 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2014, 2016 Attila Molnar <attilamolnar@hush.com>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "inspircd.h"
#include "modules/isupport.h"

/** This class manages the generation and transmission of ISUPPORT. */
class ISupportManager
{
 private:
	/** The generated numerics which are sent to clients. */
	std::vector<Numeric::Numeric> cachednumerics;

	/** The tokens which were generated by the last update. */
	ISupport::TokenMap cachedtokens;

	/** Provider for the ISupport::EventListener event. */
	ISupport::EventProvider isupportevprov;

	/** Escapes an ISUPPORT token value and appends it to the buffer.
	 * @param buffer The buffer to append to.
	 * @param value An ISUPPORT token value.
	 */
	void AppendValue(std::string& buffer, const std::string& value);

	/** Builds the ISUPPORT numerics from a list of tokens.
	 * @param tokens The tokens to build from.
	 * @param numerics The location to store the built numeric lists.
	 */
	void BuildNumerics(ISupport::TokenMap& tokens, std::vector<Numeric::Numeric>& numerics);

 public:
	ISupportManager(Module* mod);

	/** (Re)build the ISUPPORT vector.
	 * Called by the core on boot after all modules have been loaded, and every time when a module is loaded
	 * or unloaded. Calls the OnBuildISupport hook, letting modules manipulate the ISUPPORT tokens.
	 */
	void Build();

	/** Send the 005 numerics (ISUPPORT) to a user.
	 * @param user The user to send the ISUPPORT numerics to
	 */
	void SendTo(LocalUser* user);
};

/** These commands require no parameters, but if there is a parameter it is a server name where the command will be routed to.
 */
class ServerTargetCommand : public Command
{
 public:
	ServerTargetCommand(Module* mod, const std::string& Name)
		: Command(mod, Name)
	{
	}

	RouteDescriptor GetRouting(User* user, const Params& parameters) override;
};

class CommandAdmin : public ServerTargetCommand
{
 public:
	std::string adminname;
	std::string admindesc;
	std::string adminemail;
	CommandAdmin(Module* parent);
	CmdResult Handle(User* user, const Params& parameters) override;
};

class CommandCommands final
	: public SplitCommand
{
 public:
	CommandCommands(Module* parent);
	CmdResult HandleLocal(LocalUser* user, const Params& parameters) override;
};

class CommandInfo : public ServerTargetCommand
{
 public:
	CommandInfo(Module* parent);
	CmdResult Handle(User* user, const Params& parameters) override;
};

class CommandModules : public ServerTargetCommand
{
 public:
	CommandModules(Module* parent);
	CmdResult Handle(User* user, const Params& parameters) override;
};

class CommandMotd : public ServerTargetCommand
{
 public:
	ConfigFileCache motds;

	CommandMotd(Module* parent);
	CmdResult Handle(User* user, const Params& parameters) override;
};

class CommandServList : public SplitCommand
{
 private:
	UserModeReference invisiblemode;

 public:
	CommandServList(Module* parent);
	CmdResult HandleLocal(LocalUser* user, const Params& parameters) override;
};

class CommandTime : public ServerTargetCommand
{
 public:
	CommandTime(Module* parent);
	CmdResult Handle(User* user, const Params& parameters) override;
};

class CommandVersion : public Command
{
 private:
	ISupportManager& isupport;

 public:
	CommandVersion(Module* parent, ISupportManager& isupportmgr);
	CmdResult Handle(User* user, const Params& parameters) override;
};
