# Project Diablo 2 BH

## Setting up Debug and Build output Folders

Set the following environment variables according to your setup, change the folder to your Diablo II folder. This can either be done in the environment variable tab, or open up powershell as administrator and run the following commands to set it system wide:

> :warning: I suggest using a clean Diablo II 1.13c folder you are not using for anything else for this, since it will override txt files in this folder when either running scripts or debugging with Visual Studio.  
> :warning: Make sure 'game.exe' has compatability mode set to Windows XP (Service Pack 2).

```
[System.Environment]::SetEnvironmentVariable('DIABLO_DEBUG_FOLDER','C:\Program Files (x86)\Diablo II\ProjectD2',[System.EnvironmentVariableTarget]::Machine)
```

You can change the command line arguments as you see fit.

```
[System.Environment]::SetEnvironmentVariable('DIABLO_DEBUG_COMMAND_LINE_ARGUMENTS','-w -ns -direct -txt',[System.EnvironmentVariableTarget]::Machine)
```

Any debug or changed version will only work in *Single Player*. Do not enter multiplayer with a modified BH

## Chat panel

BH replaces the in-game chat overlay with a tabbed panel (`BH/Modules/Chat`). It opens with the chat box (Enter) and shows the
recent lines of the selected tab, like the vanilla overlay, while you are not typing.

* Tabs: **All**, **Game** (what players in this game say, party members and your own lines included), **Whispers**
  (in and out, one tab), **System** (server text, game and party events, item notifications, BH/PD2 messages) and
  **Global** (placeholder until realm-wide chat exists). Tabs you are not looking at show an unread count.
* Scrollback: the last `history_lines` lines (shared by all tabs). Mouse wheel over the panel, PgUp/PgDn, Home (oldest), End or
  the "Jump to latest" button.
* Lines containing your character name or a `mention_keywords` entry (whole word, any case) are highlighted, with an optional
  sound (`mention_sound`, a Sounds.txt row).
* While typing: Up/Down recall your last 50 sent lines (the unsent text comes back at the end), Ctrl+Tab / Ctrl+Shift+Tab switch
  tabs. Ctrl+R (`reply_hotkey`) starts a whisper to whoever whispered you last.
* Commands typed in the chat box: `.chat on|off` (off = vanilla chat), `.chat ts` (timestamps), `.chat tab <name>`,
  `.chat clear`, `.chat status`.

`BH.json` section `chat`: `enabled`, `timestamps`, `mention_highlight`, `mention_sound`, `mention_keywords`, `history_lines`,
`visible_rows`, `width`, `fade_seconds`, `fade_rows`, `always_open`, `reply_hotkey`.
