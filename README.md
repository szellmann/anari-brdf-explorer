ANARI BRDF Explorer
===================

The BRDF explorer is a re-imagination of [WDAS's original BRDF explorer tool][1]
using ANARI. It was developed as a project for the [ANARI Hackathon][2] in October 2024.
The tool can as of now load user-defined BRDFs as C++ plug-ins (see the [plugins](/plugins)
folder for an example of how to do this). Through BRDF parameter introspection (user
implements the functions `querySupportedSubtypes()` (the BRDF names) and
`querySupportedParams()` (list of accepted parameters)) an (Im-)GUI to manipulate BRDFs
is dynamically created. The BRDF explorer currently supports visualizing the BRDF surface
through the `eval()` function; support for sampling is already there but is currently
commented inside the code. This would be easy to hook up but I didn't find the time
during the hackathon.

[1]: https://github.com/wdas/brdf
[2]: https://www.khronos.org/events/anari-hackathon-2024
