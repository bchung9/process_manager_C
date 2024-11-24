# How to Run the Process Manager Code:
Step 1: Install a C Compiler
-	Option 1: Install MinGW
-	Option 2: Install Visual Studio Code (for Windows)
-	
Step 2: Install Windows API
-	If using MinGW, there is no need to install Windows API (built-in).
-	If using Visual Studio Code, install Windows SDK alongside C/C++ tools.
-	
Step 3: Create a C Program File
1.	Use any text editor
2.	Copy the C code then paste it.
3.	Save and name the file “process_manager.c”.
4.	
Step 4: Compile the Code
1.	Open Command Prompt (CMD).
2.	Run the following command “gcc process_manager.c -o process_manager.exe -lgdi32 -luser32”.
3.	Run the executable.
