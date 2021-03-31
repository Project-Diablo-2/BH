project(BH)

set(SOURCE_FILES "AsyncDrawBuffer.cpp"
                 "BH.cpp"
                 "BitReader.cpp"
                 "Common.cpp"
                 "Config.cpp"
                 "D2Handlers.cpp"
                 "D2Helpers.cpp"
                 "D2Intercepts.cpp"
                 "D2Stubs.cpp"
                 "D2Version.cpp"
                 "DllMain.cpp"
                 "JSONObject.cpp"
                 "MPQInit.cpp"
                 "MPQReader.cpp"
                 "Mustache.cpp"
                 "Patch.cpp"
                 "TableReader.cpp"
                 "Task.cpp")

set(MODULE_SOURCES "Modules/Module.cpp"
                   "Modules/ModuleManager.cpp"
                   "Modules/AutoTele/AutoTele.cpp"
                   "Modules/Bnet/Bnet.cpp"
                   "Modules/ChatColor/ChatColor.cpp"
                   "Modules/Gamefilter/Gamefilter.cpp"
                   "Modules/Item/Item.cpp"
                   "Modules/Item/ItemDisplay.cpp"
                   "Modules/ItemMover/ItemMover.cpp"
                   "Modules/Maphack/Maphack.cpp"
                   "Modules/Party/Party.cpp"
                   "Modules/ScreenInfo/ScreenInfo.cpp"
                   "Modules/StashExport/StashExport.cpp")

set(DRAWING_SOURCES "Drawing/Hook.cpp"
                    "Drawing/Advanced/Checkhook/Checkhook.cpp"
                    "Drawing/Advanced/Colorhook/Colorhook.cpp"
                    "Drawing/Advanced/Combohook/Combohook.cpp"
                    "Drawing/Advanced/Inputhook/Inputhook.cpp"
                    "Drawing/Advanced/Keyhook/Keyhook.cpp"
                    "Drawing/Basic/Boxhook/Boxhook.cpp"
                    "Drawing/Basic/Crosshook/Crosshook.cpp"
                    "Drawing/Basic/Framehook/Framehook.cpp"
                    "Drawing/Basic/Linehook/Linehook.cpp"
                    "Drawing/Basic/Texthook/Texthook.cpp"
                    "Drawing/Stats/StatsDisplay.cpp"
                    "Drawing/UI/UI.cpp"
                    "Drawing/UI/UITab.cpp")

set(SOURCE_FILES ${SOURCE_FILES} ${MODULE_SOURCES} ${DRAWING_SOURCES})
add_library(BH ${SOURCE_FILES})
target_include_directories(BH PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/cpp-lru-cache/include)
target_link_libraries(BH ${STORM_LIBRARY} Shlwapi)
