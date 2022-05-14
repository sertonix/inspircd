/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2019 linuxdaemon <linuxdaemon.irc@gmail.com>
 *   Copyright (C) 2014 Googolplexed <googol@googolplexed.net>
 *   Copyright (C) 2013, 2018, 2020-2021 Sadie Powell <sadie@witchery.services>
 *   Copyright (C) 2012-2014 Attila Molnar <attilamolnar@hush.com>
 *   Copyright (C) 2012 Robby <robby@chatbelgie.be>
 *   Copyright (C) 2012 Boleslaw Tokarski <boleslaw.tokarski@tieto.com>
 *   Copyright (C) 2010 Daniel De Graaf <danieldg@inspircd.org>
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


#include "inspircd.h"
#include "modules/account.h"

class ModulePassForward final
	: public Module
{
private:
	Account::API accountapi;
	std::string nickrequired, forwardmsg, forwardcmd;

public:
	ModulePassForward()
		: Module(VF_VENDOR, "Allows an account password to be forwarded to a services pseudoclient such as NickServ.")
		, accountapi(this)
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		auto tag = ServerInstance->Config->ConfValue("passforward");
		nickrequired = tag->getString("nick", "NickServ");
		forwardmsg = tag->getString("forwardmsg", "NOTICE $nick :*** Forwarding password to $nickrequired");
		forwardcmd = tag->getString("cmd", "SQUERY $nickrequired :IDENTIFY $nick $pass", 1);
	}

	void FormatStr(const LocalUser* user, const std::string& format, const std::string& pass, std::string& result)
	{
		for (unsigned int i = 0; i < format.length(); i++)
		{
			char c = format[i];
			if (c == '$')
			{
				if (!format.compare(i, 13, "$nickrequired", 13))
				{
					result.append(nickrequired);
					i += 12;
				}
				else if (!format.compare(i, 5, "$nick", 5))
				{
					result.append(user->nick);
					i += 4;
				}
				else if (!format.compare(i, 5, "$user", 5))
				{
					result.append(user->ident);
					i += 4;
				}
				else if (!format.compare(i, 5, "$pass", 5))
				{
					result.append(pass);
					i += 4;
				}
				else
					result.push_back(c);
			}
			else
				result.push_back(c);
		}
	}

	void OnPostConnect(User* ruser) override
	{
		LocalUser* user = IS_LOCAL(ruser);
		if (!user || user->password.empty())
			return;

		// If the connect class requires a password, don't forward it
		if (!user->GetClass()->config->getString("password").empty())
			return;

		if (accountapi && accountapi->GetAccountName(user))
		{
			// User is logged in already (probably via SASL) don't forward the password
			return;
		}

		ForwardPass(user, user->password);
	}

	void OnPostCommand(Command* command, const CommandBase::Params& parameters, LocalUser* user, CmdResult result, bool loop) override
	{
		if (command->name == "NICK" && parameters.size() > 1)
			ForwardPass(user, parameters[1]);
	}

	void ForwardPass(LocalUser* user, const std::string& pass)
	{
		if (!nickrequired.empty())
		{
			/* Check if nick exists and is on a services server. */
			User* u = ServerInstance->Users.Find(nickrequired);
			if (!u || !u->server->IsService())
				return;
		}

		std::string tmp;
		if (!forwardmsg.empty())
		{
			FormatStr(user, forwardmsg, pass, tmp);
			ServerInstance->Parser.ProcessBuffer(user, tmp);
			tmp.clear();
		}

		FormatStr(user, forwardcmd, pass, tmp);
		ServerInstance->Parser.ProcessBuffer(user, tmp);
	}
};

MODULE_INIT(ModulePassForward)
