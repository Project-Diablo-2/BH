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

## Unit tests

`tests/BH.Tests.vcxproj` builds a console program that runs BH's game-independent logic (loot filter rules, formulas, config, stash export templates and more) against small fakes of the game functions it calls. It is not part of `BH.sln`, so building BH is unchanged. CI builds and runs it on every pull request.

```
msbuild tests\BH.Tests.vcxproj /p:Configuration=Release /p:Platform=Win32
tests\bin\Release\BH.Tests.exe
```

Or open `tests\BH.Tests.vcxproj` in Visual Studio and run it. Run `BH.Tests.exe --help` for filtering options (for example `-ts=LootFilterRules` runs one suite). Tests marked `should_fail` document known bugs: they fail today, and the run turns red once the bug is fixed so the marker can be removed.
