# KSP-RO-TOOLS

KSP-RO-TOOLS is a C-based (because fast) tool for interplanetary trajectory planning, mission planning, and launch simulations. This tool was mainly created as a support for Kerbal Space Program's Realism Overhaul in combination with its addon Realistic Progression One (RP-1). It provides features which seem to be not available (at least not using a 5 minute Google-search) but can be helpful for managing and going through a KSP RP-1 Career. These features include:

### Interplanetary Transfer Planner
Simple transfers from Planet A to Planet B (no intermediate swing-bys) are relatively simple to determine and a mod for this already exists in KSP ([Transfer Window Planner](https://forum.kerbalspaceprogram.com/topic/84005-112x-transfer-window-planner-v1800-april-11/)) as well as multiple tools on the internet. But going to the outer planets can be quite costly on its own, which is why many space probes like JUICE or Europa Clipper (to name the latest two) use swing-bys with the inner planets to gain extra energy and thereby saving a lot of fuel. A few tools on the internet like [this](https://kerbal-transfer-illustrator.netlify.app/Flyby) are already available to provide possible (and argueably not very efficient) multi-swing-by transfers if you know what you are looking for (incl. the exact swing-by itinerary).

The interplanetary transfer planner of this tool makes the search for transfers less restrictive with also providing more efficient transfers. The only things someone really needs to know is the planet to launch from, the destination planet, the minimum and maximum launch date (e.g. 1960 to 1970) and the maximum travel duration. Optionally you may also want to have a maximum dv cost limit. If with these set constraints there are any transfers, it will give you all the options that are available (if you try to get a fly-by at Neptune in less than 300 days and a maximum departure delta-v of 'you might reach Mars' m/s, don't be surprised if you don't get far. What did you expect?)

It will store all the possible options into a file for you to load into the analysis tool where you can determine the best option for you using some filters and transfer information and store that best transfer in a file.

This tool can also visualize any stored transfer and get the respective transfer information. If a good time frame to choose from is not known before the calculations, this tool can also be used to play around with transfers and trajectories by creating manual itineraries to get a broad picture of the state of the solar system during any time frames. All the calculations in this tool are done using simple keplerian orbits. If you are interested in the transfer properties in a more accurate n-body physics simulation, a [GMAT](https://sourceforge.net/projects/gmat/) script can be created which will determine the transfer in the system not neglecting the planets gravitational influence.

The interplanetary transfer planner was validated against the mission trajectories of Voyager I/II, Cassini-Huygens and Europa Clipper.

(The method of doing two consecutive swing-bys at the same planet as used by Cassini-Huygens is still in development as efficient in-flight manoeuvres are orders of magnitude more costly to determine. It can be accessed inside the tool but don't use it unless there is a small available time frame and a fixed trajectory already determined.)

### At the Moment Inaccessible or Planned Features
- Rocket Launch simulator including launch profile optimization (calculations fully functional but needs database to work)
- Mission planner to determine transfer parameters and transfer costs from an initial orbit to a defined final orbit (yet to be implemented)
- Database for missions, launcher, etc. (work in progress)

## Usage
TBD

## Documentation
TBD

## Literature
Most of the orbital mechanics calculations are were inspired by and can be found in Curtis' _Orbital Mechanics for Engineering Students_.

## License
See the [LICENSE](LICENSE) file for details.
