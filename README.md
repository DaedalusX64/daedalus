
# DaedalusX64  [![Discord Badge]][Discord Link]

An actively developed **N64 Emulator** for `Linux` <br>
and `PSP` with many experimental optimizations.

*Ports for `Windows` / `Mac` / `PS Vita` are* ***planned*** *.*

<br>

---

**⸢ [Releases] ⸥ ⸢ [Wiki] ⸥**

---

<br>

## Features

- **Fast Emulation**
- **High Compatibility**
- **PSP TV 480p Support**

<br>


Our **PSP** port is noteworthy for being the **fastest** <br>
N64 emulator on the platform, achieving close to, <br>
to even full speed in many titles.

<br>

---

## Installation

1. Download the latest **[Release][Releases]**.

2. Plug your **PSP** into your computer.

3. Navigate to `/PSP/GAME/`.

4. Create a folder with the name `daedalus`.

5. Place the `EBOOT.PBP` into this folder.

6. Copy your **ROMs** into `daedalus/Roms`.

    *They will automatically appear in* ***Daedalus*** *.*

<br>

##### Note

*If the downloaded release is a* ***ZIP*** *file with a folder* <br>
*containing a `EBOOT.PBP` file, after extraction, simply* <br>
*drag & drop that extracted folder into `/PSP/GAME/`.*

<br>

---

## Building

<br>

#### Windows

1. **Clone** the repository.

2. Open the project in `Visual Studio 2019`.

3. Use the `Build All` option.

<br>

#### Linux & MAC

1. **Clone** the repository.

2. **Run** `build_daedalus.sh` in a terminal.

    *Select your OS by passing it as a parameter.*

    ```sh
    ./build_daedalus.sh LINUX
    ```

<br>

---

## Credits

<br>

`kreationz` [`salvy6735`] [`Corn`] `Chilly Willy` <br>

  **Original DaedalusX64 Code**

  <br>

[`Wally`]

  *Optimizations* <br>
  *Improvements* <br>
  *Ports*

[`z2442`]

  *Compilation Improvements* <br>
  *Optimizations* <br>
  *Updating*

[`mrneo240`]

  *Compilation Help* <br>
  *Optimizations*

[`TheMrIron2`]

  *Wiki Maintenance* <br>
  *Optimizations*


<!----------------------------------------------------------------------------->

[Wiki]: https://github.com/DaedalusX64/daedalus/wiki
[Releases]: https://github.com/DaedalusX64/daedalus/releases

[Discord Badge]: https://img.shields.io/badge/Discord-7289DA?style=for-the-badge&logo=discord&logoColor=white&style=flat
[Discord Link]: https://discord.gg/FrVTpBV

<!----------------------------------------------------------------------------->

[`Wally`]: https://github.com/wally4000
[`z2442`]: https://github.com/z2442
[`salvy6735`]: https://github.com/salvy
[`TheMrIron2`]: https://github.com/TheMrIron2
[`Corn`]: https://github.com/CornN64
[`mrneo240`]: https://github.com/mrneo240
