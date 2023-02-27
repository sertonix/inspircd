/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2023 Sadie Powell <sadie@witchery.services>
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
#include "duration.h"
#include "modules/stats.h"
#include "modules/whois.h"

class HelpOp final
	: public SimpleUserMode
{
public:
	std::vector<User*> helpers;

	HelpOp(Module* mod)
		: SimpleUserMode(mod, "helpop", 'h', true)
	{
	}

	bool OnModeChange(User* source, User* dest, Channel* channel, Modes::Change& change) override
	{
		if (!SimpleUserMode::OnModeChange(source, dest, channel, change))
			return false;

		if (change.adding)
			helpers.push_back(dest);
		else
			stdalgo::erase(helpers, dest);

		return true;
	}
};

class ModuleHelpMode final
	: public Module
	, public Stats::EventListener
	, public Whois::EventListener
{
private:
	bool ignorehideoper;
	HelpOp helpop;
	UserModeReference hideoper;

public:
	ModuleHelpMode()
		: Module(VF_VENDOR, "Adds user mode h (helpop) which marks a server operator as being available for help.")
		, Stats::EventListener(this, 50)
		, Whois::EventListener(this)
		, helpop(this)
		, hideoper(this, "hideoper")
	{
	}

	void ReadConfig(ConfigStatus& status) override
	{
		const auto& tag = ServerInstance->Config->ConfValue("helpmode");
		ignorehideoper = tag->getBool("ignorehideoper", false);
	}

	ModResult OnStats(Stats::Context& stats) override
	{
		if (stats.GetSymbol() != 'P')
			return MOD_RES_PASSTHRU;

		for (auto* helper : helpop.helpers)
		{
			if (helper->server->IsService())
				continue; // Ignore services.

			if (helper->IsOper() && (!ignorehideoper || !helper->IsModeSet(hideoper)))
				continue; // Ignore helpers.

			std::string extra;
			if (helper->IsAway())
			{
				const std::string awayperiod = Duration::ToString(ServerInstance->Time() - helper->awaytime);
				const std::string awaytime = InspIRCd::TimeString(helper->awaytime);

				extra = INSP_FORMAT(": away for {} [since {}] ({})", awayperiod, awaytime, helper->awaymsg);
			}

			auto* lhelper = IS_LOCAL(helper);
			if (lhelper)
			{
				const std::string idleperiod = Duration::ToString(ServerInstance->Time() - lhelper->idle_lastmsg);
				const std::string idletime = InspIRCd::TimeString(lhelper->idle_lastmsg);

				extra += INSP_FORMAT("{} idle for {} [since {}]",  extra.empty() ? ':' : ',', idleperiod, idletime);
			}

			stats.AddGenericRow(INSP_FORMAT("\x02{}\x02 ({}){}", helper->nick, helper->GetRealUserHost(), extra));
		}

		// Allow the core to add normal opers.
		return MOD_RES_PASSTHRU;
	}

	void OnWhois(Whois::Context& whois) override
	{
		if (whois.GetTarget()->IsModeSet(helpop))
			whois.SendLine(RPL_WHOISHELPOP, "is available for help.");
	}
};

MODULE_INIT(ModuleHelpMode)
