# arduino electric car sketch
Arduino sketch to act as a state machine for a small electrical toy car such as a peg perego or a power wheels.

Modifications to your car is done at your own risk and without any liability

Parts used are an Arduino Uno, although an arduino with similar pin layout can be used.

A 50A Dual-Channel motor drive module-Arduino Compatible controller from [elechouse](http://www.elechouse.com/elechouse/index.php?main_page=product_info&products_id=2179)
Their motor library with some modifications to include braking and coasting.
https://github.com/elechouse/motor

A metal hal sensor pedal bought on [ebay](http://www.ebay.co.uk/itm/111635811510?euid=ad05bdba2b88470db225e211aa759299&bu=43620839744&cp=1&exe=12742&ext=32470&es=3&nqc=ECAAAgAAAAAAAABACAAQAAAAAAEAAAAAAAAABIAAAAAAAAAAAAAAkAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAEAACEIAAEABAAAAAAAAABQAAAAABAAAQAIABAAAgAAAAAIAAAAAAABA*&nqt=ECAAAiAAAAAAAABACEAQAAAAEAEBAAAAgAAABIAAAAAAAQIAAAAEkAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAACAEAACEIAAEABAAAAAAAAABQAAAAABAAAQAIABAAAgAAAAAIAAAAAAABA*&ec=1&sojTags=es=es,nqc=nqc,nqt=nqt,ec=ec,exe=exe,ext=ext,bu=bu).
Metal Hall effect Scooter foot throttle pedal 'free shipping' jt2 generic


Also using the awesome [ResponsiveAnalogRead](https://github.com/dxinteractive/ResponsiveAnalogRead) library from Damien Clarke to smooth out the analog input.

The code have been written using the [PlatformIO](http://platformio.org/).
