@echo off

rem �����P�i�^�[�Q�b�g�t�@�C�����j
rem �����Q�i�\�����[�V�����f�B���N�g���j

set target_file_name=%1
set output_dir=%2
set project_name=%3

echo %target_file_name%>%output_dir%\StartUp.txt
echo %project_name%_Main>>%output_dir%\StartUp.txt