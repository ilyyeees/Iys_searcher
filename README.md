# IYS Searcher - Find Your Files, Fast!

## Hey There! 👋

Ever found yourself digging through folders, trying to remember *where* you saved that one specific file? Yeah, me too. That's exactly why I built **IYS Searcher**. My name's **Ilyes Abbas** (you might also know me online as **il.y.s** or **ilyyeess**), and I wanted a straightforward, no-nonsense tool to quickly find files on my computer without getting bogged down.

IYS Searcher is a simple desktop app crafted with C++ and the lovely Qt 6 framework. Its main goal? To help you pinpoint files based on their names, maybe filter by type, and do it all without making you wait forever or freezing up while it works its magic. Think of it as your little desktop bloodhound for files!

## What Can It Do? ✨

I poured some effort into making this useful, so here’s what IYS Searcher brings to the table:

* **Dig Through Filenames:** Just type in a part of the filename you're looking for, and it'll hunt it down.
* **Pinpoint Your Search Area:** Don't want to search *everywhere*? No problem! You can tell it exactly which folder to start digging in[cite: 2]. Or, if you're feeling adventurous (or desperate!), leave the start path blank, and it'll bravely check *all* the main drives it can access on your system[cite: 2, 3].
* **Filter by File Type:** Only interested in, say, `.txt` files or maybe `.jpg` images? Pop the extension into the filter box (like `.txt` or just `txt`), and it'll narrow down the results[cite: 2].
* **Case? What Case?** Sometimes you don't remember if it was `Report.txt` or `report.txt`. Just tick the "Case Insensitive" box, and IYS Searcher won't care about upper or lower case letters[cite: 2]. Easy!
* **Smooth Sailing GUI:** Built with Qt, the interface is pretty straightforward. No complicated menus, just the essentials to get the search going.
* **No More Freezing!** This was important to me. The actual searching happens in the background (thanks, `QThread`! [cite: 2]). That means the app stays responsive. You can move the window, click around, or even cancel the search without the whole thing locking up on you.
* **Stop! I Found It!** If the search is taking too long, or you spot the file you need fly by in the results, just hit the "Cancel Search" button to tell the worker thread to stop[cite: 2].
* **See What It Finds:** Results pop up in the main text area as they're discovered[cite: 2]. Clear and simple.
* **Keep a Record:** Got a long list of finds? You can tell IYS Searcher to save all the results (the full paths of the files found) into a text file for later reference[cite: 2].
* **Know What's Happening:** Down in the status bar, you'll see updates – which directory it's currently peeking into, how many files it's found so far, and a little progress indicator so you know it's actually working[cite: 2].
* **Peek Behind the Curtain (Optional):** Curious about *why* it couldn't access a certain folder (like pesky permission errors)? Tick the "Verbose Errors" box, and it'll report those kinds of hiccups in the results area too[cite: 2, 3].
* **A Little Flair:** It even has a custom icon and a little splash screen when it starts up, just for fun[cite: 1, 2].

## Stuff You'll Need (Dependencies) 🛠️

To build this yourself, you'll need a few things set up on your machine:

* **CMake:** Gotta have version 3.16 or newer[cite: 1]. It's the recipe book for building the app.
* **Qt Framework:** Specifically, version 6. Make sure you've got the `Core`, `Widgets`, and `Concurrent` modules installed[cite: 1]. This provides all the GUI elements and background threading tools.
* **C++ Compiler:** A compiler that understands C++17 is necessary[cite: 1]. Most modern compilers (like recent GCC, Clang, or MSVC) will do just fine.

## How to Build It (The Fun Part!) ⚙️

Got the dependencies? Awesome! Here's how to compile the code using CMake:

1.  **Get Qt Ready:** Make sure CMake can find your Qt 6 installation. It might find it automatically if it's in your system's PATH, or you might need to tell CMake where it is using `-DCMAKE_PREFIX_PATH=/path/to/Qt6`.
2.  **Open Your Terminal/Command Prompt:** Navigate to the directory where you have the IYS Searcher source code (`/path/to/searcher`).
3.  **Make a Build Directory:** It's good practice to build outside the source directory.
    ```bash
    cd /path/to/searcher
    mkdir build
    cd build
    ```
4.  **Run CMake:** Point CMake to the source directory (`..`) and optionally tell it where Qt is:
    ```bash
    cmake .. -DCMAKE_PREFIX_PATH=/path/to/your/Qt6/installation # <-- Adjust this path if needed!
    ```
5.  **Compile!** Now, use the build tool CMake set up for you:
    * If you're on **Linux or macOS** and CMake generated Makefiles (the default usually):
        ```bash
        make
        ```
    * If you're on **Windows** and CMake generated a Visual Studio solution (`.sln`): Open that solution file in Visual Studio and build the `IYSSearcher` project from there.
    * If you're using **Ninja** (a faster build tool you might have configured):
        ```bash
        ninja
        ```

If all goes well, you'll find the shiny new `IYSSearcher` (or `IYSSearcher.exe` on Windows) executable right there in your `build` directory!

## How to Use It (Easy Peasy!) 🖱️

Alright, you've built it or downloaded it. Let's find some files!

1.  **Launch the App:** Double-click that `IYSSearcher` executable.
2.  **What Are You Looking For?** Type the filename clue into the "Search Term" box[cite: 2].
3.  **Where Should It Look?**
    * Want to search *everywhere*? Just leave the "Start Path" box empty[cite: 2].
    * Have a specific folder in mind? Click the "Browse..." button next to "Start Path" and pick your directory[cite: 2].
4.  **Any Specific Type?** If you only want, say, PDF files, type `.pdf` in the "File Extension" box[cite: 2]. Leave it blank to search for any file type.
5.  **Save the Booty?** Want a list saved? Click "Browse..." next to "Output File" and choose where to save the results text file[cite: 2]. Otherwise, just leave it blank.
6.  **Tweak the Search:** Check the boxes for "Case Insensitive" or "Verbose Errors" if you need those options[cite: 2].
7.  **Go! Go! Go!** Hit the "Start Search" button[cite: 2].
8.  **Watch the Magic:** Keep an eye on the status bar at the bottom for updates and the results area for files popping up[cite: 2].
9.  **Had Enough?** If you need to stop early, just click the "Cancel Search" button[cite: 2].
10. **All Done!** When it finishes (or you cancel), the status bar will let you know, and you'll have your list of files right there in the app (and in the output file, if you chose one)[cite: 2].

## A Look Under the Hood (Code Structure) 🧑‍💻

For the curious minds, here's a quick breakdown of how the code is organized:

* `main.cpp`: This is where it all begins. It sets up the basic Qt application environment, flashes the cool splash screen, creates the main window (`MainWindow`), and shows it[cite: 1].
* `mainwindow.h` / `mainwindow.cpp`: This is the heart of the user interface[cite: 2]. It defines how the main window looks and behaves. It takes your search inputs, kicks off the search process by creating a `SearchWorker` and putting it on a separate `QThread`, listens for signals from that worker (like "found a file!" or "I'm done!"), and updates the text area and status bar accordingly. It also handles the "Cancel" button logic[cite: 2].
* `mainwindow.ui`: Just a definition file created by Qt Designer. It describes *what* widgets (buttons, text boxes, etc.) are on the main window and how they're laid out[cite: 1]. `mainwindow.cpp` brings this definition to life.
* `searchworker.h` / `searchworker.cpp`: This is the busy bee working in the background[cite: 1]. It lives on a separate thread so it doesn't block the GUI. It takes the `SearchConfig` (all your search settings) from the `MainWindow`, calls the actual search logic in `searchlogic.cpp`, handles writing to the output file if requested, checks if you've hit "Cancel", and sends signals back to the `MainWindow` to report progress, results, errors, and when it's finally finished[cite: 1].
* `searchlogic.h` / `searchlogic.cpp`: Here lies the core searching brainpower[cite: 1].
    * `SearchConfig`: A simple structure just to hold all the search settings together neatly[cite: 1].
    * `searchDirectoryRecursive`: This is the real workhorse. It dives into directories (using the modern C++ `std::filesystem` library), checks each file against your search term and extension filter, and if it finds a match, it uses a special function (a "callback") to immediately report the find back to the `SearchWorker`[cite: 1]. It also handles skipping directories it can't access and calls itself to go into subdirectories.
    * `getRootPaths`: A helper function to figure out the starting points when you ask it to search *everywhere*. It uses Qt's `QStorageInfo` to find all the drives/mount points it can[cite: 1].
* `CMakeLists.txt`: The master build instructions file for CMake. It tells CMake how to compile everything, which Qt modules are needed, and how to link them all together to create the final executable[cite: 1].
* `resources.qrc`: A small Qt file that bundles things like the application icon (`search_icon.png`) and splash screen image (`splash_screen.png`) directly into the program itself, so you don't need separate image files sitting next to the executable[cite: 1].

## Made With <3 By...

* **Ilyes Abbas** (il.y.s / ilyyeess)

    *(Living in Algeria)*

## License Info

This project is licensed under the **MIT License**. You can find the full license text in the `LICENSE` file in this repository. Feel free to reach out if you have questions about usage.

---

Hope this gives you a better feel for the project! Let me know if you have any questions.

Contact me on Discord for any questions!

add me by username : **il.y.s**
or just type **<@201682329851265026>** in any chat and my contact will pop up!
