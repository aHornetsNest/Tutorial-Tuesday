_  = function(p) return p; end;
name = _('AHNCockpit');
Description = 'Game Res: 2560*3544 // Monitor Setup 1) 2560/1440 2) 1920/1080 top aligned, right of display1 3) 600/1024 left aligned 4) 1024/600 left aligned  5)480/480 left aligned '
Viewports =
{
     Center =
     {
          x = 0;
          y = 0;
          width = 2560;
          height = 1440;
          viewDx = 0;
          viewDy = 0;
          aspect = 16 / 9;
     }
}

CENTER_MFCD =
{
     x = 0;
     y = 1864; -- commence y value 424px below start of screen y origin position.
     width = 600;
     height = 600;
}

FA_18C_IFEI =
{
     x = -480; -- commence x value -480px relative to start of screen x origin position. Do not place any display left of IFEI in config.
     y = 2484; -- commence y value 20px below start of screen y origin position.
     width = 1480;
     height = 590;
}

RWR =
{
     x = 0;
     y = 1450; -- commence y value 10px below start of screen y origin position.
     width = 480;
     height = 480;
}


UIMainView = Viewports.Center
GU_MAIN_VIEWPORT = Viewports.Center