# KMAT

***KMAT*** (**K**MAT's a **M**ission **A**nalysis **T**ool) is a C-based (because fast) tool for interplanetary transfer planning, mission planning, and launch simulations. This tool was mainly created as a fun learning experience for me and for supporting my Kerbal Space Program Career in Realism Overhaul in combination with its add-on Realistic Progression One (RP-1). 
It has since become a tool that is not limited to Realism Overhaul and should be able to be used by anyone who is somewhat familiar with basic concepts from orbital mechanics. The features this tool incorporates include:
- Interplanetary transfer planner for multi-swing by missions (released)
- Rocket Launch simulator including launch profile optimization (calculations fully functional but needs database to work; inaccessible)
- Mission planner to determine transfer parameters and transfer costs from an initial orbit to a defined final orbit (yet to be implemented)
- Database for missions, launcher, etc. (work in progress and needs database to work; inaccessible)

### Interplanetary Transfer Planner
Simple transfers from Planet A to Planet B (no intermediate swing-bys) are relatively simple to determine and a mod for this already exists in KSP ([Transfer Window Planner](https://forum.kerbalspaceprogram.com/topic/84005-112x-transfer-window-planner-v1800-april-11/)) as well as multiple tools on the internet. But going to the outer planets can be quite costly on its own, which is why many space probes like JUICE or Europa Clipper (to name just the latest two) use swing-bys with the inner planets to gain extra energy and thereby saving a lot of fuel. A few tools on the internet like [this](https://kerbal-transfer-illustrator.netlify.app/Flyby) are already available to provide possible (but argueably not very efficient) multi-swing-by transfers if you know what you are looking for (incl. the exact swing-by sequence).

The interplanetary transfer planner of this tool makes the search for transfers less restrictive while also providing more efficient transfers (if available). The transfers are purely ballistic, meaning no deep-space maneuvers between swing-bys. The only aspects someone really needs to know is the planet to launch from, the destination planet, the minimum and maximum launch date (e.g. 1960 to 1970) and the maximum travel duration. Optionally you may also want to have a maximum dv cost limit. If with these set constraints there are any transfers, it will give you a good range of options that are available (if you try to get a fly-by at Neptune in less than 300 days and a maximum departure delta-v of 'you might reach Mars' m/s, don't be surprised if you don't get far. What did you expect?)

It will store all the possible options into a file for you to load into the analysis tool where you can determine the best option for you using some filters and transfer information and store that best transfer in a file.

The interplanetary transfer planner was validated against the mission trajectories of Voyager I/II, Cassini-Huygens and Europa Clipper, and against respective transfers using NASA's [General Mission Analysis Tool (GMAT)](https://sourceforge.net/projects/gmat/).

(The method of doing two consecutive swing-bys at the same planet as used by Cassini-Huygens is still in development, because efficient in-flight manoeuvres are orders of magnitude more costly to determine.)

## Usage
### Installation
* **Windows:**\
Download `kmat-win.zip` from the [latest release](https://github.com/Zange10/ksp-ro-tools/releases) and extract it (to a location of your choice). Now you can run the application using the `Launch_KMAT.exe`. On initial startup you might need to tell windows that this application is safe to run.

* **Linux:**\
Download `kmat-linux.tar.gz` from the [latest release](https://github.com/Zange10/ksp-ro-tools/releases) and extract it (to a location of your choice). To be able to run, the packages for [GTK3](https://www.gtk.org/docs/installations/linux/) and [SQLite3](https://www.sqlite.org/index.html) need to be installed. You can run the application using the `Launch_KMAT` executable.

### KMAT System Config Creator
If the system you are playing with is not available or not detailed enough, you can let the _System Config Creator_ create a config file of your KSP System:
1. Move the content of `KmatSystemConfigCreator.zip` to KSP's `GameData` folder
2. Launch KSP and create a new save
3. Go into the Tracking station

After a few seconds, the config file compatible with KMAT should appear inside the `GameData/KmatSystemConfigCreator` folder

### Solar System using Ephemeris Data
If you would like to use the solar system's ephemeris data (this might be recommended if you use Principia), download `solar_system_ephem.cfg` and move it to the `Celestial_Systems` folder. On initial startup, the ephemeris data will be retrieved from [JPL's Horizon API](https://ssd.jpl.nasa.gov/horizons/app.html#/) and stored in the `Ephemerides` folder (created automatically).

### Usage of KMAT for "relevant" Projects
If the tool is used for a "relevant" project (e.g. content creation or uni project), don't hesitate to reach out to me for a quick heads-up. That way I know the tool is being used for something actually useful.

## About this tool
First and foremost, this tool is a fun learning experience for me, and it has been on and off for the past couple of years. I started developing this tool to make my life easier in KSP-RP1. Initially it started out as a rocket launch simulation program for analysis of launchers regarding their capabilities and optimal launch profiles and also included a small calculator for simple dv-transfer estimates (e.g. Hohmann).

## Documentation
[Wiki](https://github.com/Zange10/ksp-ro-tools/wiki)

## Literature
Most of the orbital mechanics calculations were inspired by and can be found in Curtis' _Orbital Mechanics for Engineering Students_ or Vallado's _Fundamentals of Astrodynamics and Applications_.

## License
See the [LICENSE](LICENSE) file for details.
