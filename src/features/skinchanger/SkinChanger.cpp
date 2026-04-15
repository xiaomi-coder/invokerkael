#include "../../Includes.h"

// -----------------------------------------------------------------------
// Skin databases (PaintKit ID, Display Name)
// Only the most popular/iconic skins per weapon are listed.
// PaintKit 0 = Default (no skin change)
// -----------------------------------------------------------------------

// ===================== AK-47 =====================
const SkinInfo_t SkinChanger::g_AK47Skins[] = {
    { 0,    "Default"             },
    { 801,  "Asiimov"             },
    { 180,  "Fire Serpent"        },
    { 675,  "Neon Rider"          },
    { 600,  "Bloodsport"          },
    { 524,  "Fuel Injector"       },
    { 302,  "Vulcan"              },
    { 282,  "Redline"             },
    { 474,  "Aquamarine Revenge"  },
    { 490,  "Point Disarray"      },
    { 380,  "Wasteland Rebel"     },
    { 44,   "Case Hardened"       },
    { 226,  "Elite Build"         },
    { 639,  "The Empress"         },
    { 506,  "Neon Revolution"     },
    { 122,  "Emerald Pinstripe"   },
    { 731,  "Phantom Disruptor"   },
    { 14,   "Red Laminate"        },
    { 456,  "Hydroponic"          },
    { 172,  "Safari Mesh"         },
    { 316,  "Jaguar"              },
    { 870,  "Ice Coaled"          },
    { 1018, "Inheritance"         },
    { 926,  "Slate"               },
    { 940,  "Nightwish"           },
    { 940,  "Head Shot"           },
    { 1064, "Panthera onca"       },
};
const int SkinChanger::g_nAK47SkinCount = sizeof(g_AK47Skins) / sizeof(g_AK47Skins[0]);

// ===================== M4A4 =====================
const SkinInfo_t SkinChanger::g_M4A4Skins[] = {
    { 0,    "Default"             },
    { 309,  "Howl"                },
    { 632,  "Buzz Kill"           },
    { 533,  "Desolate Space"      },
    { 480,  "Royal Paladin"       },
    { 400,  "The Battlestar"      },
    { 255,  "Asiimov"             },
    { 164,  "Modern Hunter"       },
    { 336,  "Dragon King"         },
    { 471,  "Evil Daimyo"         },
    { 615,  "Hellfire"            },
    { 587,  "Neo-Noir"            },
    { 844,  "Cyber Security"      },
    { 1020, "Temukau"             },
    { 973,  "The Emperor"         },
};
const int SkinChanger::g_nM4A4SkinCount = sizeof(g_M4A4Skins) / sizeof(g_M4A4Skins[0]);

// ===================== M4A1-S =====================
const SkinInfo_t SkinChanger::g_M4A1SSkins[] = {
    { 0,    "Default"             },
    { 548,  "Decimator"           },
    { 445,  "Golden Coil"         },
    { 587,  "Mecha Industries"    },
    { 430,  "Hyper Beast"         },
    { 360,  "Cyrex"               },
    { 321,  "Atomic Alloy"        },
    { 254,  "Guardian"            },
    { 681,  "Leaded Glass"        },
    { 631,  "Chantico's Fire"     },
    { 714,  "Nightmare"           },
    { 612,  "Golden Coil"         },
    { 845,  "Player Two"          },
    { 362,  "Knight"              },
    { 440,  "Icarus Fell"         },
    { 644,  "Flashback"           },
    { 910,  "Printstream"         },
    { 987,  "Emphorosaur-S"       },
};
const int SkinChanger::g_nM4A1SSkinCount = sizeof(g_M4A1SSkins) / sizeof(g_M4A1SSkins[0]);

// ===================== AWP =====================
const SkinInfo_t SkinChanger::g_AWPSkins[] = {
    { 0,    "Default"             },
    { 344,  "Dragon Lore"         },
    { 279,  "Asiimov"             },
    { 475,  "Hyper Beast"         },
    { 488,  "Medusa"              },
    { 662,  "Fever Dream"         },
    { 584,  "Oni Taiji"           },
    { 424,  "Man-o-war"           },
    { 446,  "Graphite"            },
    { 227,  "Redline"             },
    { 174,  "Safari Mesh"         },
    { 640,  "PAW"                 },
    { 756,  "Wildfire"            },
    { 803,  "Containment Breach"  },
    { 662,  "Neo-Noir"            },
    { 51,   "Lightning Strike"    },
    { 84,   "Corticera"           },
    { 227,  "Electric Hive"       },
    { 174,  "BOOM"                },
    { 1025, "Chromatic Aberration"},
    { 967,  "Duality"             },
    { 936,  "Fade"                },
};
const int SkinChanger::g_nAWPSkinCount = sizeof(g_AWPSkins) / sizeof(g_AWPSkins[0]);

// ===================== Desert Eagle =====================
const SkinInfo_t SkinChanger::g_DeagleSkins[] = {
    { 0,    "Default"             },
    { 351,  "Conspiracy"          },
    { 527,  "Kumicho Dragon"      },
    { 37,   "Blaze"               },
    { 711,  "Code Red"            },
    { 469,  "Directive"           },
    { 185,  "Crimson Web"         },
    { 232,  "Cobalt Disruption"   },
    { 274,  "Pilot"               },
    { 469,  "Sunset Storm"        },
    { 351,  "Naga"                },
    { 841,  "Printstream"         },
    { 662,  "Mecha Industries"    },
    { 938,  "Fennec Fox"          },
    { 987,  "Ocean Drive"         },
};
const int SkinChanger::g_nDeagleSkinCount = sizeof(g_DeagleSkins) / sizeof(g_DeagleSkins[0]);

// ===================== Glock-18 =====================
const SkinInfo_t SkinChanger::g_GlockSkins[] = {
    { 0,    "Default"             },
    { 353,  "Water Elemental"     },
    { 48,   "Fade"                },
    { 623,  "Weasel"              },
    { 551,  "Off World"           },
    { 437,  "Twilight Galaxy"     },
    { 680,  "Bullet Queen"        },
    { 578,  "Steel Disruption"    },
    { 230,  "Brass"               },
    { 160,  "Dragon Tattoo"       },
    { 413,  "Grinder"             },
    { 917,  "Gamma Doppler"       },
    { 1006, "Winterized"          },
};
const int SkinChanger::g_nGlockSkinCount = sizeof(g_GlockSkins) / sizeof(g_GlockSkins[0]);

// ===================== USP-S =====================
const SkinInfo_t SkinChanger::g_USPSSkins[] = {
    { 0,    "Default"             },
    { 313,  "Orion"               },
    { 653,  "Neo-Noir"            },
    { 657,  "Cortex"              },
    { 504,  "Kill Confirmed"      },
    { 318,  "Caiman"              },
    { 290,  "Guardian"            },
    { 454,  "Lead Conduit"        },
    { 637,  "Blueprint"           },
    { 704,  "Flashback"           },
    { 183,  "Dark Water"          },
    { 236,  "Overgrowth"          },
    { 60,   "Stainless"           },
    { 911,  "Printstream"         },
    { 989,  "Black Lotus"         },
};
const int SkinChanger::g_nUSPSSkinCount = sizeof(g_USPSSkins) / sizeof(g_USPSSkins[0]);

// ===================== AUG =====================
const SkinInfo_t SkinChanger::g_AUGSkins[] = {
    { 0,    "Default"             },
    { 455,  "Akihabara Accept"    },
    { 300,  "Chameleon"           },
    { 708,  "Momentum"            },
    { 545,  "Stymphalian"         },
    { 305,  "Bengal Tiger"        },
};
const int SkinChanger::g_nAUGSkinCount = sizeof(g_AUGSkins) / sizeof(g_AUGSkins[0]);

// ===================== SSG 08 =====================
const SkinInfo_t SkinChanger::g_SSG08Skins[] = {
    { 0,    "Default"             },
    { 503,  "Blood in Water"      },
    { 624,  "Dragonfire"          },
    { 319,  "Abyss"               },
    { 233,  "Dark Water"          },
};
const int SkinChanger::g_nSSG08SkinCount = sizeof(g_SSG08Skins) / sizeof(g_SSG08Skins[0]);

// ===================== FAMAS =====================
const SkinInfo_t SkinChanger::g_FAMASSkins[] = {
    { 0,    "Default"             },
    { 523,  "Mecha Industries"    },
    { 626,  "Roll Cage"           },
    { 312,  "Styx"                },
    { 249,  "Afterimage"          },
};
const int SkinChanger::g_nFAMASSkinCount = sizeof(g_FAMASSkins) / sizeof(g_FAMASSkins[0]);

// ===================== Galil AR =====================
const SkinInfo_t SkinChanger::g_GALILSkins[] = {
    { 0,    "Default"             },
    { 428,  "Eco"                 },
    { 379,  "Chatterbox"          },
    { 237,  "Blue Titanium"       },
    { 294,  "Cerberus"            },
};
const int SkinChanger::g_nGALILSkinCount = sizeof(g_GALILSkins) / sizeof(g_GALILSkins[0]);

// ===================== SG 553 =====================
const SkinInfo_t SkinChanger::g_SG553Skins[] = {
    { 0,    "Default"             },
    { 487,  "Cyrex"               },
    { 602,  "Phantom"             },
    { 305,  "Tiger Moth"          },
};
const int SkinChanger::g_nSG553SkinCount = sizeof(g_SG553Skins) / sizeof(g_SG553Skins[0]);

// ===================== SCAR-20 =====================
const SkinInfo_t SkinChanger::g_SCAR20Skins[] = {
    { 0,    "Default"             },
    { 165,  "Splash Jam"          },
    { 312,  "Cardiac"             },
    { 597,  "Bloodsport"          },
};
const int SkinChanger::g_nSCAR20SkinCount = sizeof(g_SCAR20Skins) / sizeof(g_SCAR20Skins[0]);

// ===================== G3SG1 =====================
const SkinInfo_t SkinChanger::g_G3SG1Skins[] = {
    { 0,    "Default"             },
    { 511,  "The Executioner"     },
    { 311,  "Chronos"             },
    { 465,  "Stinger"             },
};
const int SkinChanger::g_nG3SG1SkinCount = sizeof(g_G3SG1Skins) / sizeof(g_G3SG1Skins[0]);

// ===================== P90 =====================
const SkinInfo_t SkinChanger::g_P90Skins[] = {
    { 0,    "Default"             },
    { 283,  "Asiimov"             },
    { 359,  "Death by Kitty"      },
    { 311,  "Trigon"              },
    { 671,  "Astral Jormungandr"  },
    { 161,  "Blind Spot"          },
};
const int SkinChanger::g_nP90SkinCount = sizeof(g_P90Skins) / sizeof(g_P90Skins[0]);

// ===================== MP9 =====================
const SkinInfo_t SkinChanger::g_MP9Skins[] = {
    { 0,    "Default"             },
    { 510,  "Airlock"             },
    { 262,  "Rose Iron"           },
    { 690,  "Hydra"               },
};
const int SkinChanger::g_nMP9SkinCount = sizeof(g_MP9Skins) / sizeof(g_MP9Skins[0]);

// ===================== MAC-10 =====================
const SkinInfo_t SkinChanger::g_MAC10Skins[] = {
    { 0,    "Default"             },
    { 433,  "Neon Rider"          },
    { 482,  "Malachite"           },
    { 351,  "Heat"                },
};
const int SkinChanger::g_nMAC10SkinCount = sizeof(g_MAC10Skins) / sizeof(g_MAC10Skins[0]);

// ===================== UMP-45 =====================
const SkinInfo_t SkinChanger::g_UMP45Skins[] = {
    { 0,    "Default"             },
    { 556,  "Primal Saber"        },
    { 279,  "Blaze"               },
    { 556,  "Arctic Wolf"         },
};
const int SkinChanger::g_nUMP45SkinCount = sizeof(g_UMP45Skins) / sizeof(g_UMP45Skins[0]);

// ===================== MP7 =====================
const SkinInfo_t SkinChanger::g_MP7Skins[] = {
    { 0,    "Default"             },
    { 479,  "Nemesis"             },
    { 352,  "Ocean Foam"          },
    { 661,  "Bloodsport"          },
};
const int SkinChanger::g_nMP7SkinCount = sizeof(g_MP7Skins) / sizeof(g_MP7Skins[0]);

// ===================== Five-SeveN =====================
const SkinInfo_t SkinChanger::g_FiveSevenSkins[] = {
    { 0,    "Default"             },
    { 427,  "Monkey Business"     },
    { 352,  "Copper Galaxy"       },
    { 586,  "Hyper Beast"         },
    { 810,  "Angry Mob"           },
};
const int SkinChanger::g_nFiveSevenSkinCount = sizeof(g_FiveSevenSkins) / sizeof(g_FiveSevenSkins[0]);

// ===================== Tec-9 =====================
const SkinInfo_t SkinChanger::g_TEC9Skins[] = {
    { 0,    "Default"             },
    { 614,  "Fuel Injector"       },
    { 179,  "Red Quartz"          },
    { 595,  "Decimator"           },
};
const int SkinChanger::g_nTEC9SkinCount = sizeof(g_TEC9Skins) / sizeof(g_TEC9Skins[0]);

// ===================== P250 =====================
const SkinInfo_t SkinChanger::g_P250Skins[] = {
    { 0,    "Default"             },
    { 551,  "Asiimov"             },
    { 269,  "Mehndi"              },
    { 388,  "Muertos"             },
    { 164,  "Undertow"            },
};
const int SkinChanger::g_nP250SkinCount = sizeof(g_P250Skins) / sizeof(g_P250Skins[0]);

// ===================== CZ75-Auto =====================
const SkinInfo_t SkinChanger::g_CZ75Skins[] = {
    { 0,    "Default"             },
    { 270,  "Victoria"            },
    { 298,  "Tigris"              },
    { 453,  "Pole Position"       },
};
const int SkinChanger::g_nCZ75SkinCount = sizeof(g_CZ75Skins) / sizeof(g_CZ75Skins[0]);

// ===================== Dual Berettas =====================
const SkinInfo_t SkinChanger::g_DualBerettasSkins[] = {
    { 0,    "Default"             },
    { 565,  "Cobalt Quartz"       },
    { 266,  "Marina"              },
    { 651,  "Twin Turbo"          },
};
const int SkinChanger::g_nDualBerettasSkinCount = sizeof(g_DualBerettasSkins) / sizeof(g_DualBerettasSkins[0]);

// ===================== P2000 =====================
const SkinInfo_t SkinChanger::g_P2000Skins[] = {
    { 0,    "Default"             },
    { 389,  "Fire Elemental"      },
    { 168,  "Ocean Foam"          },
    { 277,  "Amber Fade"          },
    { 493,  "Imperial Dragon"     },
};
const int SkinChanger::g_nP2000SkinCount = sizeof(g_P2000Skins) / sizeof(g_P2000Skins[0]);

// ===================== R8 Revolver =====================
const SkinInfo_t SkinChanger::g_R8Skins[] = {
    { 0,    "Default"             },
    { 548,  "Fade"                },
    { 523,  "Amber Fade"          },
    { 562,  "Reboot"              },
    { 722,  "Skull Crusher"       },
};
const int SkinChanger::g_nR8SkinCount = sizeof(g_R8Skins) / sizeof(g_R8Skins[0]);

// ===================== Knife Skins (universal) =====================
const SkinInfo_t SkinChanger::g_KnifeSkins[] = {
    { 0,    "Default"             },
    { 38,   "Fade"                },
    { 59,   "Slaughter"           },
    { 44,   "Case Hardened"       },
    { 12,   "Crimson Web"         },
    { 415,  "Doppler Phase 1"     },
    { 416,  "Doppler Phase 2"     },
    { 417,  "Doppler Phase 3"     },
    { 418,  "Doppler Phase 4"     },
    { 419,  "Doppler Ruby"        },
    { 420,  "Doppler Sapphire"    },
    { 421,  "Doppler Black Pearl" },
    { 568,  "Gamma Doppler P1"    },
    { 569,  "Gamma Doppler P2"    },
    { 570,  "Gamma Doppler P3"    },
    { 571,  "Gamma Doppler P4"    },
    { 572,  "Gamma Doppler Emerald"},
    { 409,  "Tiger Tooth"         },
    { 413,  "Marble Fade"         },
    { 558,  "Lore"                },
    { 98,   "Ultraviolet"         },
    { 410,  "Damascus Steel"      },
    { 414,  "Rust Coat"           },
    { 43,   "Night"               },
    { 42,   "Blue Steel"          },
    { 40,   "Urban Masked"        },
    { 175,  "Scorched"            },
    { 5,    "Forest DDPAT"        },
    { 77,   "Boreal Forest"       },
    { 135,  "Vanilla"             },
    { 411,  "Autotronic"          },
    { 1119, "Gamma Doppler P1"    },
};
const int SkinChanger::g_nKnifeSkinCount = sizeof(g_KnifeSkins) / sizeof(g_KnifeSkins[0]);

// -----------------------------------------------------------------------
// Master weapon category table for GUI
// -----------------------------------------------------------------------
WeaponCategory_t SkinChanger::g_vecWeaponCategories[] = {
    { "AK-47",          WEAPON_AK_47,          g_AK47Skins,          g_nAK47SkinCount          },
    { "M4A4",           WEAPON_M4A4,           g_M4A4Skins,          g_nM4A4SkinCount          },
    { "M4A1-S",         WEAPON_M4A1_S,         g_M4A1SSkins,         g_nM4A1SSkinCount         },
    { "AWP",            WEAPON_AWP,             g_AWPSkins,           g_nAWPSkinCount           },
    { "Desert Eagle",   WEAPON_DESERT_EAGLE,    g_DeagleSkins,        g_nDeagleSkinCount        },
    { "Glock-18",       WEAPON_GLOCK_18,        g_GlockSkins,         g_nGlockSkinCount         },
    { "USP-S",          WEAPON_USP_S,           g_USPSSkins,          g_nUSPSSkinCount          },
    { "AUG",            WEAPON_AUG,             g_AUGSkins,           g_nAUGSkinCount           },
    { "SSG 08",         WEAPON_SSG_08,          g_SSG08Skins,         g_nSSG08SkinCount         },
    { "FAMAS",          WEAPON_FAMAS,           g_FAMASSkins,         g_nFAMASSkinCount         },
    { "Galil AR",       WEAPON_GALIL_AR,        g_GALILSkins,         g_nGALILSkinCount         },
    { "SG 553",         WEAPON_SG_553,          g_SG553Skins,         g_nSG553SkinCount         },
    { "SCAR-20",        WEAPON_SCAR_20,         g_SCAR20Skins,        g_nSCAR20SkinCount        },
    { "G3SG1",          WEAPON_G3SG1,           g_G3SG1Skins,         g_nG3SG1SkinCount         },
    { "P90",            WEAPON_P90,             g_P90Skins,           g_nP90SkinCount           },
    { "MP9",            WEAPON_MP9,             g_MP9Skins,           g_nMP9SkinCount           },
    { "MAC-10",         WEAPON_MAC_10,          g_MAC10Skins,         g_nMAC10SkinCount         },
    { "UMP-45",         WEAPON_UMP_45,          g_UMP45Skins,         g_nUMP45SkinCount         },
    { "MP7",            WEAPON_MP7,             g_MP7Skins,           g_nMP7SkinCount           },
    { "Five-SeveN",     WEAPON_FIVE_SEVEN,      g_FiveSevenSkins,     g_nFiveSevenSkinCount     },
    { "Tec-9",          WEAPON_TEC_9,           g_TEC9Skins,          g_nTEC9SkinCount          },
    { "P250",           WEAPON_P250,            g_P250Skins,          g_nP250SkinCount          },
    { "CZ75-Auto",      WEAPON_CZ75_AUTO,       g_CZ75Skins,          g_nCZ75SkinCount          },
    { "Dual Berettas",  WEAPON_DUAL_BERETTAS,   g_DualBerettasSkins,  g_nDualBerettasSkinCount  },
    { "P2000",          WEAPON_P2000,           g_P2000Skins,         g_nP2000SkinCount         },
    { "R8 Revolver",    WEAPON_R8_REVOLVER,     g_R8Skins,            g_nR8SkinCount            },
    { "Knife",          WEAPON_KNIFE_CT,        g_KnifeSkins,         g_nKnifeSkinCount         },
};
int SkinChanger::g_nWeaponCategoryCount = sizeof(g_vecWeaponCategories) / sizeof(g_vecWeaponCategories[0]);

// -----------------------------------------------------------------------
// Get or create a config for a given weapon definition index
// -----------------------------------------------------------------------
WeaponSkinConfig_t& SkinChanger::GetWeaponConfig(std::uint16_t nDefIndex)
{
    return m_mapWeaponConfigs[nDefIndex];
}

// -----------------------------------------------------------------------
// Helper: check if a definition index is a knife
// -----------------------------------------------------------------------
static bool IsKnifeDefIndex(std::uint16_t idx)
{
    switch (idx)
    {
    case WEAPON_KNIFE_GG:
    case WEAPON_KNIFE_CT:
    case WEAPON_KNIFE_T:
    case WEAPON_BAYONET:
    case WEAPON_CLASSIC_KNIFE:
    case WEAPON_FLIP_KNIFE:
    case WEAPON_GUT_KNIFE:
    case WEAPON_KARAMBIT:
    case WEAPON_M9_BAYONET:
    case WEAPON_HUNTSMAN_KNIFE:
    case WEAPON_FALCHION_KNIFE:
    case WEAPON_BOWIE_KNIFE:
    case WEAPON_BUTTERFLY_KNIFE:
    case WEAPON_SHADOW_DAGGERS:
    case WEAPON_PARACORD_KNIFE:
    case WEAPON_SURVIVAL_KNIFE:
    case WEAPON_URSUS_KNIFE:
    case WEAPON_NAVAJA_KNIFE:
    case WEAPON_NOMAD_KNIFE:
    case WEAPON_STILETTO_KNIFE:
    case WEAPON_TALON_KNIFE:
    case WEAPON_SKELETON_KNIFE:
    case WEAPON_KUKRI_KNIFE:
        return true;
    default:
        return false;
    }
}

// -----------------------------------------------------------------------
// Helper: try to find a schema offset from the map using runtime Hash
// Tries primary name first, then any number of fallback names
// -----------------------------------------------------------------------
static std::uintptr_t ResolveSchemaOffset(std::initializer_list<const char*> names)
{
    for (const char* szName : names)
    {
        auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szName));
        if (it != SchemaSystem::m_mapSchemaOffsets.end() && it->second != 0)
            return it->second;
    }
    return 0;
}

// -----------------------------------------------------------------------
// Apply skins to the local player's weapons
// Called from TickThread every tick
//
// Schema names may change between CS2 updates. We try multiple variants
// for each field to maximize compatibility.
// -----------------------------------------------------------------------
void SkinChanger::Run()
{
    if (!m_bEnabled) return;

    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn) return;

    if (reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000) return;
    if (!pLocalPawn->IsAlive()) return;

    CCSPlayer_WeaponServices* pWeaponServices = pLocalPawn->m_pWeaponServices();
    if (!pWeaponServices || reinterpret_cast<std::uintptr_t>(pWeaponServices) < 0x1000) return;

    C_NetworkUtlVectorBaseSimple hWeapons = pWeaponServices->m_hMyWeapons();
    if (hWeapons.m_nSize <= 0 || hWeapons.m_nSize > 16 || hWeapons.m_pData < 0x1000) return;

    // Cache schema offsets — resolved once with multiple fallback alternatives
    static std::once_flag flagResolve;
    static std::uintptr_t uAttrMgr = 0, uItem = 0;
    static std::uintptr_t uItemIDHigh = 0, uFallbackPaint = 0, uFallbackSeed = 0;
    static std::uintptr_t uFallbackWear = 0, uFallbackStatTrak = 0, uItemDefIdx = 0;
    static bool bResolved = false;
    // If fallback fields are on the entity itself (not in EconItemView)
    static bool bFallbackOnEntity = false;

    std::call_once(flagResolve, []()
    {
        uAttrMgr = ResolveSchemaOffset({
            "C_EconEntity->m_AttributeManager",
            "CEconEntity->m_AttributeManager"
        });
        uItem = ResolveSchemaOffset({
            "C_AttributeContainer->m_Item",
            "CAttributeContainer->m_Item"
        });
        uItemIDHigh = ResolveSchemaOffset({
            "C_EconItemView->m_iItemIDHigh",
            "CEconItemView->m_iItemIDHigh"
        });
        uItemDefIdx = ResolveSchemaOffset({
            "C_EconItemView->m_iItemDefinitionIndex",
            "CEconItemView->m_iItemDefinitionIndex"
        });

        // Fallback paint kit — try many variants
        uFallbackPaint = ResolveSchemaOffset({
            "C_EconItemView->m_nFallbackPaintKit",
            "CEconItemView->m_nFallbackPaintKit",
            "C_EconEntity->m_nFallbackPaintKit",
            "CEconEntity->m_nFallbackPaintKit"
        });
        uFallbackSeed = ResolveSchemaOffset({
            "C_EconItemView->m_nFallbackSeed",
            "CEconItemView->m_nFallbackSeed",
            "C_EconEntity->m_nFallbackSeed",
            "CEconEntity->m_nFallbackSeed"
        });
        uFallbackWear = ResolveSchemaOffset({
            "C_EconItemView->m_flFallbackWear",
            "CEconItemView->m_flFallbackWear",
            "C_EconEntity->m_flFallbackWear",
            "CEconEntity->m_flFallbackWear"
        });
        uFallbackStatTrak = ResolveSchemaOffset({
            "C_EconItemView->m_nFallbackStatTrak",
            "CEconItemView->m_nFallbackStatTrak",
            "C_EconEntity->m_nFallbackStatTrak",
            "CEconEntity->m_nFallbackStatTrak"
        });

        // Check if fallback fields are on the entity (C_EconEntity) level
        // If so, we need to offset from weapon base, not EconItemView
        {
            auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash("C_EconEntity->m_nFallbackPaintKit"));
            if (it != SchemaSystem::m_mapSchemaOffsets.end() && it->second != 0)
                bFallbackOnEntity = true;
            auto it2 = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash("CEconEntity->m_nFallbackPaintKit"));
            if (it2 != SchemaSystem::m_mapSchemaOffsets.end() && it2->second != 0)
                bFallbackOnEntity = true;
        }

        bResolved = (uAttrMgr != 0 && uItem != 0);

        // Always print diagnostics to console (not just Debug)
        std::cout << "  [SKIN] Schema offset resolution:" << std::endl;
        std::cout << "  [SKIN]   AttributeManager: 0x" << std::hex << uAttrMgr << std::endl;
        std::cout << "  [SKIN]   Item:             0x" << uItem << std::endl;
        std::cout << "  [SKIN]   ItemIDHigh:       0x" << uItemIDHigh << std::endl;
        std::cout << "  [SKIN]   ItemDefIdx:       0x" << uItemDefIdx << std::endl;
        std::cout << "  [SKIN]   FallbackPaint:    0x" << uFallbackPaint << std::endl;
        std::cout << "  [SKIN]   FallbackSeed:     0x" << uFallbackSeed << std::endl;
        std::cout << "  [SKIN]   FallbackWear:     0x" << uFallbackWear << std::endl;
        std::cout << "  [SKIN]   FallbackStatTrak: 0x" << uFallbackStatTrak << std::endl;
        std::cout << "  [SKIN]   FallbackOnEntity: " << (bFallbackOnEntity ? "YES" : "NO") << std::endl;
        std::cout << "  [SKIN]   Resolved:         " << (bResolved ? "YES" : "FAILED") << std::dec << std::endl;

        if (uFallbackPaint == 0)
        {
            std::cout << "  [SKIN] WARNING: FallbackPaintKit not found! Scanning schema map..." << std::endl;
            // Dump all schema entries containing "Fallback" or "PaintKit" for debugging
            for (const auto& [hash, offset] : SchemaSystem::m_mapSchemaOffsets)
            {
                // We can't reverse the hash, but we can show any offsets that might be relevant
                // by checking if common known field names match
                static const char* probeNames[] = {
                    "C_EconItemView->m_nFallbackPaintKit",
                    "CEconItemView->m_nFallbackPaintKit",
                    "C_EconEntity->m_nFallbackPaintKit",
                    "C_BaseEntity->m_nFallbackPaintKit",
                };
                for (const char* probe : probeNames)
                {
                    if (FNV1A::Hash(probe) == hash)
                        std::cout << "  [SKIN]   FOUND: " << probe << " = 0x" << std::hex << offset << std::dec << std::endl;
                }
            }
        }
    });

    // If we don't have the base offsets, cannot proceed
    if (!bResolved) return;

    // If we don't have fallback paint kit offset, we can't apply skins
    if (uFallbackPaint == 0) return;

    for (int i = 0; i < hWeapons.m_nSize; i++)
    {
        CHandle<C_BasePlayerWeapon> hWeaponHandle = g_Memory.ReadMemory<CHandle<C_BasePlayerWeapon>>(hWeapons.m_pData + (i * 0x4));
        if (!hWeaponHandle.IsValid()) continue;

        C_BasePlayerWeapon* pWeapon = hWeaponHandle.Get();
        if (!pWeapon || reinterpret_cast<std::uintptr_t>(pWeapon) < 0x1000) continue;
        std::uintptr_t uWeaponAddr = reinterpret_cast<std::uintptr_t>(pWeapon);

        // Read item definition index (always through AttributeManager->Item path)
        std::uint16_t nDefIndex = 0;
        if (uItemDefIdx > 0)
            nDefIndex = g_Memory.ReadMemory<std::uint16_t>(uWeaponAddr + uAttrMgr + uItem + uItemDefIdx);

        // --- DEBUG LOGGING ---
        static bool bDumpedOnce = false;
        if (!bDumpedOnce && pWeaponServices && pWeaponServices->m_hActiveWeapon().Get() == pWeapon)
        {
            std::ofstream debugFile("skinchanger_debug.txt");
            if (debugFile.is_open())
            {
                debugFile << "Active Weapon Diagnostic:\n";
                debugFile << "uWeaponAddr: 0x" << std::hex << uWeaponAddr << "\n";
                debugFile << "uAttrMgr: 0x" << uAttrMgr << "\n";
                debugFile << "uItem: 0x" << uItem << "\n";
                debugFile << "uItemDefIdx: 0x" << uItemDefIdx << "\n";
                debugFile << "nDefIndex: " << std::dec << nDefIndex << "\n";
                debugFile << "iItemIDHigh: 0x" << std::hex << uItemIDHigh << "\n";
                debugFile << "iAccountID: 0x" << (SchemaSystem::m_mapSchemaOffsets[FNV1A::Hash("C_EconItemView->m_iAccountID")]) << "\n";
                debugFile << "FallbackPaint: 0x" << uFallbackPaint << "\n";
                debugFile << "FallbackOnEntity: " << (bFallbackOnEntity ? "YES" : "NO") << "\n";
                
                int readItemHigh = g_Memory.ReadMemory<int>(uWeaponAddr + uAttrMgr + uItem + uItemIDHigh);
                debugFile << "Current ItemIDHigh Value: " << std::dec << readItemHigh << "\n";
                debugFile.close();
                bDumpedOnce = true;
            }
        }
        // --- END DEBUG LOGGING ---

        if (nDefIndex == 0) continue;

        // Check if we have a skin config for this weapon
        std::uint16_t nLookupKey = IsKnifeDefIndex(nDefIndex) ? (std::uint16_t)WEAPON_KNIFE_CT : nDefIndex;

        auto it = m_mapWeaponConfigs.find(nLookupKey);
        if (it == m_mapWeaponConfigs.end()) continue;

        WeaponSkinConfig_t& cfg = it->second;
        if (cfg.m_nPaintKit == 0) continue; // Default = skip

        // Use a flag to track if we changed anything this tick and must force mesh updates
        bool bNeedsVisualUpdate = false;

        // Base address for EconItemView (where all Fallback fields are stored in CS2)
        std::uintptr_t uEconBase = uWeaponAddr + uAttrMgr + uItem;

        // Read current paint kit on the 3rd person weapon
        int iCurrentPaintKit = 0;
        if (uFallbackPaint > 0)
            iCurrentPaintKit = g_Memory.ReadMemory<int>(uEconBase + uFallbackPaint);

        if (iCurrentPaintKit != cfg.m_nPaintKit)
        {
            // Set ItemIDHigh to -1 to force application of fallbacks
            if (uItemIDHigh > 0)
            {
                g_Memory.WriteMemory<int>(uEconBase + uItemIDHigh, -1);
            }

            // Write custom fallback values ALWAYS to EconItemView
            if (uFallbackPaint > 0) g_Memory.WriteMemory<int>(uEconBase + uFallbackPaint, cfg.m_nPaintKit);
            if (uFallbackWear > 0) g_Memory.WriteMemory<float>(uEconBase + uFallbackWear, cfg.m_flWear);
            if (uFallbackSeed > 0) g_Memory.WriteMemory<int>(uEconBase + uFallbackSeed, cfg.m_nSeed);
            if (uFallbackStatTrak > 0 && cfg.m_nStatTrak >= 0) g_Memory.WriteMemory<int>(uEconBase + uFallbackStatTrak, cfg.m_nStatTrak);

            bNeedsVisualUpdate = true;
        }

        // Fetch View Model offset and pointer
        std::uintptr_t uVmAddr = 0;
        if (pWeaponServices && pLocalPawn && pWeaponServices->m_hActiveWeapon().Get() == pWeapon)
        {
            auto pVmServices = pLocalPawn->m_pViewModelServices();
            if (pVmServices)
            {
                C_BaseModelEntity* pViewModel = pVmServices->m_hViewModel().Get();
                if (pViewModel && reinterpret_cast<std::uintptr_t>(pViewModel) > 0x1000)
                {
                    uVmAddr = reinterpret_cast<std::uintptr_t>(pViewModel);
                    
                    int iVmPaintKit = 0;
                    if (uFallbackPaint > 0) iVmPaintKit = g_Memory.ReadMemory<int>(uVmAddr + uFallbackPaint);

                    // If View Model was recreated by game engine (e.g., weapon switch), it loses fallbacks
                    if (iVmPaintKit != cfg.m_nPaintKit)
                    {
                        if (uFallbackPaint > 0) g_Memory.WriteMemory<int>(uVmAddr + uFallbackPaint, cfg.m_nPaintKit);
                        if (uFallbackWear > 0) g_Memory.WriteMemory<float>(uVmAddr + uFallbackWear, cfg.m_flWear);
                        if (uFallbackSeed > 0) g_Memory.WriteMemory<int>(uVmAddr + uFallbackSeed, cfg.m_nSeed);
                        if (uFallbackStatTrak > 0 && cfg.m_nStatTrak >= 0) g_Memory.WriteMemory<int>(uVmAddr + uFallbackStatTrak, cfg.m_nStatTrak);
                        
                        bNeedsVisualUpdate = true;
                    }
                }
            }
        }

        // Force CS2 to visually re-render the mesh if we modified attributes
        if (bNeedsVisualUpdate)
        {
            static std::uintptr_t uGameSceneNodeOffset = 0;
            static bool bSceneResolved = false;
            if (!bSceneResolved) {
                auto itScene = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash("C_BaseEntity->m_pGameSceneNode"));
                if (itScene != SchemaSystem::m_mapSchemaOffsets.end())
                    uGameSceneNodeOffset = itScene->second;
                bSceneResolved = true;
            }

            if (uGameSceneNodeOffset > 0)
            {
                // Update 3rd-person Weapon mesh
                std::uintptr_t uSceneNode = g_Memory.ReadMemory<std::uintptr_t>(uWeaponAddr + uGameSceneNodeOffset);
                if (uSceneNode > 0x1000)
                {
                    g_Memory.WriteMemory<int>(uSceneNode + 0x160 + 0x18, 2);
                }

                // Update 1st-person View Model mesh
                if (uVmAddr > 0)
                {
                    std::uintptr_t uVmSceneNode = g_Memory.ReadMemory<std::uintptr_t>(uVmAddr + uGameSceneNodeOffset);
                    if (uVmSceneNode > 0x1000)
                    {
                        g_Memory.WriteMemory<int>(uVmSceneNode + 0x160 + 0x18, 2);
                    }
                }
            }
        }
    }
}

void SkinChanger::Initialize()
{
    // Nothing special needed; configs are stored in the map
}

