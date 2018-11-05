//-----------------------------------------------------------------------------
// The toolbar that appears at the top left of the graphics window, where the
// user can select icons with the mouse, to perform operations equivalent to
// selecting a menu item or using a keyboard shortcut.
//
// Copyright 2008-2013 Jonathan Westhues.
//-----------------------------------------------------------------------------

//
// edit: configured toolbar for doggett's purposes
//
//

#include "solvespace.h"

struct ToolIcon {
    std::string name;
    Command     command;
    const char *tooltip;
    std::shared_ptr<Pixmap> pixmap;
};

// check function of each ToolIcon
static ToolIcon Toolbar[] = {
	{ "image",           Command::IMAGE,
      N_("Sketch image from a file"),                         {} },
	{ "scale",          Command::SCALE,
      N_("Input blueprint dimensions"),           {} },
	{ "",                Command::NONE, "",               {} },
	
	{ "line",            Command::LINE_SEGMENT,
      N_("Sketch line segment"),                              {} },
	{ "bezier",          Command::CUBIC,
      N_("Sketch cubic Bezier spline"),                       {} },  
	{ "tangent-arc",     Command::TANGENT_ARC,
      N_("Create tangent arc at selected point"),             {} },
	{ "angle",           Command::ANGLE,
      N_("Constrain angle"),                                  {} },
	{ "horiz",           Command::HORIZONTAL,
      N_("Constrain to be horizontal"),                       {} },
    	{ "vert",            Command::VERTICAL,
      N_("Constrain to be vertical"),                         {} },
	{ "parallel",        Command::PARALLEL,
      N_("Constrain to be parallel or tangent"),              {} },
    	{ "perpendicular",   Command::PERPENDICULAR,
      N_("Constrain to be perpendicular"),                    {} },
    	{ "",                Command::NONE, "",               {} },
	
	{ "text",            Command::TTF_TEXT,
      N_("Sketch curves from text in a TrueType font"),       {} },
    	{ "ontoworkplane",   Command::ONTO_WORKPLANE,
      N_("Align view to active workplane"),                   {} },
};


void GraphicsWindow::ToolbarDraw(UiCanvas *canvas) {
    ToolbarDrawOrHitTest(0, 0, canvas, NULL);
}

bool GraphicsWindow::ToolbarMouseMoved(int x, int y) {
    x += ((int)width/2);
    y += ((int)height/2);

    Command nh = Command::NONE;
    bool withinToolbar = ToolbarDrawOrHitTest(x, y, NULL, &nh);
    if(!withinToolbar) nh = Command::NONE;

    if(nh != toolbarTooltipped) {
        // Don't let the tool tip move around if the mouse moves within the
        // same item.
        toolbarMouseX = x;
        toolbarMouseY = y;
        toolbarTooltipped = Command::NONE;
    }

    if(nh != toolbarHovered) {
        toolbarHovered = nh;
        SetTimerFor(1000);
        PaintGraphics();
    }
    // So if we moved off the toolbar, then toolbarHovered is now equal to
    // zero, so it doesn't matter if the tool tip timer expires. And if
    // we moved from one item to another, we reset the timer, so also okay.
    return withinToolbar;
}

bool GraphicsWindow::ToolbarMouseDown(int x, int y) {
    x += ((int)width/2);
    y += ((int)height/2);

    Command nh = Command::NONE;
    bool withinToolbar = ToolbarDrawOrHitTest(x, y, NULL, &nh);
    // They might have clicked within the toolbar, but not on a button.
    if(withinToolbar && nh != Command::NONE) {
        for(int i = 0; SS.GW.menu[i].level >= 0; i++) {
            if(nh == SS.GW.menu[i].id) {
                (SS.GW.menu[i].fn)((Command)SS.GW.menu[i].id);
                break;
            }
        }
    }
    return withinToolbar;
}

bool GraphicsWindow::ToolbarDrawOrHitTest(int mx, int my,
                                          UiCanvas *canvas, Command *menuHit)
{
    int i;
    int x = 17, y = (int)(height - 52);

    int fudge = 8;
    int h = 34*16 + 3*16 + fudge;
    int aleft = 0, aright = 66, atop = y+16+fudge/2, abot = y+16-h;

    bool withinToolbar =
        (mx >= aleft && mx <= aright && my <= atop && my >= abot);

    if(!canvas && !withinToolbar) {
        // This gets called every MouseMove event, so return quickly.
        return false;
    }

    if(canvas) {
        canvas->DrawRect(aleft, aright, atop, abot,
                        /*fillColor=*/{ 30, 30, 30, 255 },
                        /*outlineColor=*/{});
    }

    bool showTooltip = false;
    std::string tooltip;

    bool leftpos = true;
    for(ToolIcon &icon : Toolbar) {
        if(icon.name == "") { // spacer
            if(!leftpos) {
                leftpos = true;
                y -= 32;
                x -= 32;
            }
            y -= 16;

            if(canvas) {
                // Draw a separator bar in a slightly different color.
                int divw = 30, divh = 2;
                canvas->DrawRect(x+16+divw, x+16-divw, y+24+divh, y+24-divh,
                                 /*fillColor=*/{ 45, 45, 45, 255 },
                                 /*outlineColor=*/{});
            }

            continue;
        }

        if(icon.pixmap == nullptr) {
            icon.pixmap = LoadPng("icons/graphics-window/" + icon.name + ".png");
        }

        if(canvas) {
            canvas->DrawPixmap(icon.pixmap,
                               x - (int)icon.pixmap->width  / 2,
                               y - (int)icon.pixmap->height / 2);

            if(toolbarHovered == icon.command ||
               (pending.operation == Pending::COMMAND &&
                pending.command == icon.command)) {
                // Highlight the hovered or pending item.
                int boxhw = 15;
                canvas->DrawRect(x+boxhw, x-boxhw, y+boxhw, y-boxhw,
                                 /*fillColor=*/{ 255, 255, 0, 75 },
                                 /*outlineColor=*/{});
            }

            if(toolbarTooltipped == icon.command) {
                // Display the tool tip for this item; postpone till later
                // so that no one draws over us. Don't need position since
                // that's just wherever the mouse is.
                showTooltip = true;
                tooltip     = Translate(icon.tooltip);
            }
        } else {
            int boxhw = 16;
            if(mx < (x+boxhw) && mx > (x - boxhw) &&
               my < (y+boxhw) && my > (y - boxhw))
            {
                if(menuHit) *menuHit = icon.command;
            }
        }

        if(leftpos) {
            x += 32;
            leftpos = false;
        } else {
            x -= 32;
            y -= 32;
            leftpos = true;
        }
    }

    if(canvas) {
        // Do this last so that nothing can draw over it.
        if(showTooltip) {
            for(i = 0; SS.GW.menu[i].level >= 0; i++) {
                if(toolbarTooltipped == SS.GW.menu[i].id) {
                    std::string accel = MakeAcceleratorLabel(SS.GW.menu[i].accel);
                    if(!accel.empty()) {
                        tooltip += ssprintf(" (%s)", accel.c_str());
                    }
                    break;
                }
            }

            int tw = (int)canvas->canvas->GetBitmapFont()->GetWidth(tooltip) * 8 + 10,
                th = SS.TW.LINE_HEIGHT + 2;

            int ox = toolbarMouseX + 3, oy = toolbarMouseY + 3;
            canvas->DrawRect(ox, ox+tw, oy, oy+th,
                             /*fillColor=*/{ 255, 255, 150, 255 },
                             /*outlineColor=*/{ 0, 0, 0, 255 });
            canvas->DrawBitmapText(tooltip, ox+5, oy+4, { 0, 0, 0, 255 });
        }
    }

    return withinToolbar;
}

void GraphicsWindow::TimerCallback() {
    SS.GW.toolbarTooltipped = SS.GW.toolbarHovered;
    PaintGraphics();
}

