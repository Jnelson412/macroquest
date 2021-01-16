/*
 * MacroQuest2: The extension platform for EverQuest
 * Copyright (C) 2002-2019 MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "pch.h"
#include "MQ2DataTypes.h"

namespace mq::datatypes {

enum class InvSlotMembers
{
	Pack = 1,
	Slot,
	ID,
	Name,
	Item,
};

MQ2InvSlotType::MQ2InvSlotType() : MQ2Type("invslot")
{
	ScopedTypeMember(InvSlotMembers, Pack);
	ScopedTypeMember(InvSlotMembers, Slot);
	ScopedTypeMember(InvSlotMembers, ID);
	ScopedTypeMember(InvSlotMembers, Name);
	ScopedTypeMember(InvSlotMembers, Item);
}

// FIXME: Items
// item slots:
// 2000-2015 bank window
// 2500-2503 shared bank
// 5000-5031 loot window
// 3000-3015 trade window (including npc) 3000-3007 are your slots, 3008-3015 are other character's slots
// 4000-4010 world container window
// 6000-6080 merchant window
// 7000-7080 bazaar window
// 8000-8031 inspect window
bool MQ2InvSlotType::GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest)
{
	MQTypeMember* pMember = MQ2InvSlotType::FindMember(Member);
	if (!pMember)
		return false;

	int nInvSlot = VarPtr.Int;

	switch (static_cast<InvSlotMembers>(pMember->ID))
	{
	case InvSlotMembers::ID:
		Dest.DWord = VarPtr.Int;
		Dest.Type = pIntType;
		return true;

	case InvSlotMembers::Item: {
		Dest.Type = pItemType;

		PcProfile* pProfile = GetPcProfile();
		if (!pProfile) return false;
		if (nInvSlot < 0) return false;

		CHARINFO* pCharInfo = GetCharInfo();
		if (!pCharInfo) return false;

		// TODO: We could translate this "nInvSlot" index into a ItemGlobalIndex and perform a single lookup.

		if (nInvSlot < NUM_INV_SLOTS)
		{
			if (Dest.Ptr = pProfile->InventoryContainer.GetItem(nInvSlot).get())
				return true;
		}
		else
		{
			if (nInvSlot >= 262 && nInvSlot < 342)
			{
				int nPack = (nInvSlot - 262) / 10;
				int nSlot = (nInvSlot - 262) % 10;

				if (ItemPtr pPack = pProfile->InventoryContainer.GetItem(nPack))
				{
					ItemDefinition* itemDef = pPack->GetItemDefinition();
					if (itemDef->Type == ITEMTYPE_PACK)
					{
						if (Dest.Ptr = pPack->GetChildItemContainer()->GetItem(nSlot).get())
							return true;
					}
				}
			}
			else if (nInvSlot >= 2032 && nInvSlot < 2272)
			{
				int nPack = (nInvSlot - 2032) / 10;
				int nSlot = (nInvSlot - 2) % 10;

				if (ItemPtr pPack = pCharInfo->BankItems.GetItem(nPack))
				{
					ItemDefinition* itemDef = pPack->GetItemDefinition();
					if (itemDef->Type == ITEMTYPE_PACK)
					{
						if (Dest.Ptr = pPack->GetChildItemContainer()->GetItem(nSlot).get())
							return true;
					}
				}
			}
			else if (nInvSlot >= 2532 && nInvSlot < 2552)
			{
				int nPack = 23 + ((nInvSlot - 2532) / 10);
				int nSlot = (nInvSlot - 2) % 10;

				if (ItemPtr pPack = pCharInfo->BankItems.GetItem(nPack))
				{
					ItemDefinition* itemDef = pPack->GetItemDefinition();
					if (itemDef->Type == ITEMTYPE_PACK)
					{
						if (Dest.Ptr = pPack->GetChildItemContainer()->GetItem(nSlot).get())
							return true;
					}
				}
			}
			// Bank slots (2000-2024)
			else if (nInvSlot >= 2000 && nInvSlot < 2000 + NUM_BANK_SLOTS)
			{
				if (Dest.Ptr = pCharInfo->BankItems.GetItem(nInvSlot - 2000).get())
					return true;
			}
			// Shared bank slots
			else if (nInvSlot >= 2500 && nInvSlot < 2500 + NUM_SHAREDBANK_SLOTS)
			{
				if (Dest.Ptr = pCharInfo->SharedBankItems.GetItem(nInvSlot - 2500).get())
					return true;
			}
			// 3000-3015 trade window (including npc) 3000-3007 are your slots, 3008-3015 are other character's slots
			else if (nInvSlot >= 3000 && nInvSlot < 3016)
			{
				CInvSlotWnd* pInvSlotWnd = nullptr;

				if (pGiveWnd && pGiveWnd->IsVisible())
				{
					int slot = std::min(nInvSlot - 3000, MAX_GIVE_SLOTS);
					if (slot >= MAX_TRADE_SLOTS)
						slot = 0;

					pInvSlotWnd = pGiveWnd->pInvSlotWnd[slot];
				}
				else if (pTradeWnd && pTradeWnd->IsVisible())
				{
					int slot = std::min(nInvSlot - 3000, MAX_TRADE_SLOTS);
					if (slot >= MAX_TRADE_SLOTS)
						slot = 0;

					pInvSlotWnd = pTradeWnd->pInvSlotWnd[slot];
				}

				if (pInvSlotWnd && pInvSlotWnd->pInvSlot)
				{
					if (Dest.Ptr = pInvSlotWnd->pInvSlot->GetItem().get())
						return true;
				}
			}
			// 4000-4010 world container window
			else if (nInvSlot >= 4000 && nInvSlot <= 4010) // enviro slots
			{
				uint32_t index = nInvSlot - 4000;

				if (pContainerMgr && pContainerMgr->WorldContainer)
				{
					if (Dest.Ptr = pContainerMgr->WorldContainer->GetHeldItem(index).get())
						return true;
				}
			}
			else if (nInvSlot == 4100) // enviro container
			{
				if (pContainerMgr)
				{
					if (Dest.Ptr = pContainerMgr->WorldContainer.get())
						return true;
				}
			}
		}
		return false;
	}

	case InvSlotMembers::Pack:
		Dest.DWord = 0;
		Dest.Type = pInvSlotType;
		if (nInvSlot >= 262 && nInvSlot < 342)
		{
			Dest.DWord = ((nInvSlot - 262) / 10) + InvSlot_FirstBagSlot;
			return true;
		}

		if (nInvSlot >= 2032 && nInvSlot < 2272)
		{
			Dest.DWord = ((nInvSlot - 2032) / 10) + 2000;
			return true;
		}

		if (nInvSlot >= 2532 && nInvSlot < 2552)
		{
			Dest.DWord = ((nInvSlot - 2532) / 10) + 2500;
			return true;
		}
		return false;

	case InvSlotMembers::Slot:
		Dest.DWord = 0;
		Dest.Type = pIntType;
		if (nInvSlot >= 262 && nInvSlot < 342)
		{
			Dest.DWord = (nInvSlot - 262) % 10;
			return true;
		}

		if (nInvSlot >= 2032 && nInvSlot < 2272)
		{
			Dest.DWord = (nInvSlot - 2032) % 10;
			return true;
		}

		if (nInvSlot >= 2532 && nInvSlot < 2552)
		{
			Dest.DWord = (nInvSlot - 2532) % 10;
			return true;
		}
		return false;

	case InvSlotMembers::Name:
		Dest.Type = pStringType;
		if (nInvSlot >= InvSlot_FirstWornItem && nInvSlot <= InvSlot_LastWornItem)
		{
			strcpy_s(DataTypeTemp, szItemSlot[nInvSlot]);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= InvSlot_FirstBagSlot && nInvSlot <= InvSlot_LastBagSlot)
		{
			sprintf_s(DataTypeTemp, "pack%d", nInvSlot - InvSlot_FirstBagSlot + 1);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 2000 && nInvSlot < 2024) // FIXME: Items
		{
			sprintf_s(DataTypeTemp, "bank%d", nInvSlot - 1999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 2500 && nInvSlot < 2502)
		{
			sprintf_s(DataTypeTemp, "sharedbank%d", nInvSlot - 2499);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 5000 && nInvSlot < 5032)
		{
			sprintf_s(DataTypeTemp, "loot%d", nInvSlot - 4999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 3000 && nInvSlot < 3009)
		{
			sprintf_s(DataTypeTemp, "trade%d", nInvSlot - 2999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 4000 && nInvSlot < 4009)
		{
			sprintf_s(DataTypeTemp, "enviro%d", nInvSlot - 3999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot == 4100) // its the worldcontainer
		{
			if (pContainerMgr)
			{
				if (ItemDefinition* pItemInfo = pContainerMgr->pWorldContainer->GetItemDefinition())
				{
					strcpy_s(DataTypeTemp, pItemInfo->Name);
					Dest.Ptr = &DataTypeTemp[0];
					return true;
				}
			}

			return false;
		}

		if (nInvSlot >= 6000 && nInvSlot < 6080)
		{
			sprintf_s(DataTypeTemp, "merchant%d", nInvSlot - 5999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 7000 && nInvSlot < 7089)
		{
			sprintf_s(DataTypeTemp, "bazaar%d", nInvSlot - 6999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}

		if (nInvSlot >= 8000 && nInvSlot < 8031)
		{
			sprintf_s(DataTypeTemp, "inspect%d", nInvSlot - 7999);
			Dest.Ptr = &DataTypeTemp[0];
			return true;
		}
		return false;

	default: break;
	}

	return false;
}

bool MQ2InvSlotType::dataInvSlot(const char* szIndex, MQTypeVar& Ret)
{
	if (!szIndex[0])
		return false;

	if (IsNumber(szIndex))
	{
		Ret.DWord = GetIntFromString(szIndex, 0);
		Ret.Type = pInvSlotType;
		return true;
	}
	else
	{
		char Temp[MAX_STRING] = { 0 };
		strcpy_s(Temp, szIndex);
		_strlwr_s(Temp);
		Ret.DWord = 0;

		if (ItemSlotMap.find(Temp) != ItemSlotMap.end())
		{
			Ret.DWord = ItemSlotMap[Temp];
		}

		if (Ret.DWord || !_stricmp(Temp, "charm"))
		{
			Ret.Type = pInvSlotType;
			return true;
		}
	}

	return false;
}

bool MQ2InvSlotType::ToString(MQVarPtr VarPtr, char* Destination)
{
	_itoa_s(VarPtr.Int, Destination, MAX_STRING, 10);
	return true;
}

bool MQ2InvSlotType::FromData(MQVarPtr& VarPtr, MQTypeVar& Source)
{
	VarPtr.DWord = Source.DWord;
	return true;
}

bool MQ2InvSlotType::FromString(MQVarPtr& VarPtr, const char* Source)
{
	if (IsNumber(Source))
	{
		VarPtr.DWord = GetIntFromString(Source, 0);
		return true;
	}
	else
	{
		char Temp[MAX_STRING] = { 0 };
		strcpy_s(Temp, Source);
		_strlwr_s(Temp);
		VarPtr.DWord = ItemSlotMap[Temp];
		if (VarPtr.DWord || !_stricmp(Temp, "charm"))
		{
			return true;
		}
	}
	return false;
}

} // namespace mq::datatypes
