# Racjin compression and decompression algorithms (CFC.DIG/CDDATA.DIG)
In this repository you can find code for _compression_ and _decompression_ algorithms used in old PS2/PSP games by _Racjin_ (and other Japanese companies), such as: 
* Naruto: Uzumaki Chronicles series (PS2)
* Fullmetal Alchemist 3 (PS2)
* Bleach: Soul Carnival 2 (PSP)
* Naruto Shippuden: Legends: Akatsuki Rising (PSP)
* Naruto Shippuden: Ultimate Ninja Impact (PSP)
* and many others.

Main contents of those games can be found in **CFC.DIG** or **CDDATA.DIG** files. [Here is the tool](https://github.com/Raw-man/Racjin-de-compression/releases/) for unpacking them. 

**Usage**:

Drop the file onto the executable or run the program from the command line.

>file_path <record_section_start_offset\> <record_section_end_offset\> <archive_type\> <is_big_endian\>
>
>>racjin-decompression /path/to/archive
>
>>racjin-decompression /path/to/archive 0x0 0x500 0 0

**Note**:

* In Naruto Shippuden: Ultimate Ninja Impact CFC.DIG files are encrypted and stored in PGD files so you need to decrypt them first.
* CFC.DIG and CDDATA.DIG files are structured differently.
* They have at least 3 possible structure variations.
* Their endianness differs depending on the platform.
* Sometimes it might be difficult to determine the number of file records they contain and their type. You should inspect the archives and possibly provide extra arguments to the program.

