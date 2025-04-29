@echo off

rem 引数１（ターゲットファイル名）
rem 引数２（ソリューションディレクトリ）

set target_file_name=%1
set output_dir=%2
set project_name=%3

echo %target_file_name%>%output_dir%\StartUp.txt
echo %project_name%_Main>>%output_dir%\StartUp.txt