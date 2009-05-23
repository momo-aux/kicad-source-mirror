/*************************/
/* PCBNEW: main program  */
/*************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"

#include <wx/file.h>
#include <wx/snglinst.h>

#include "common.h"
#include "pcbnew.h"
#include "plot_common.h"
#include "gestfich.h"
#include "pcbplot.h"
#include "autorout.h"
#include "cell.h"
#include "worksheet.h"
#include "zones.h"

#include "protos.h"

#include "drag.h"

#include "eda_dde.h"

bool Drc_On = TRUE;
bool g_AutoDeleteOldTrack = TRUE;
bool g_No_Via_Route;
bool g_Drag_Pistes_On;
bool g_Show_Ratsnest;
bool g_Show_Module_Ratsnest;
bool g_Show_Pads_Module_in_Move = TRUE;
bool g_Raccord_45_Auto = TRUE;
bool g_ShowIsolDuringCreateTrack;
bool Track_45_Only;
bool Segments_45_Only;
bool g_TwoSegmentTrackBuild = TRUE;
bool g_HightLigt_Status;
extern PARAM_CFG_BASE* ParamCfgList[];

char* buf_work = NULL;      /* pointeur sur le buffer de travail */
char* adr_lowmem = NULL;    /* adresse de base memoire de calcul disponible*/
char* adr_max = NULL;       /* adresse haute maxi utilisee pour la memoire */

int Angle_Rot_Module;
int ModuleSegmentWidth;
int ModuleTextWidth;
int Route_Layer_TOP;
int Route_Layer_BOTTOM;
int g_MaxLinksShowed;
int g_MagneticPadOption = capture_cursor_in_track_tool;
int g_MagneticTrackOption = capture_cursor_in_track_tool;
int g_HightLigth_NetCode = -1;

wxSize ModuleTextSize;       /* Default footprint texts size */
wxPoint g_Offset_Module;     /* Offset de trace du modul en depl */
wxString g_Current_PadName;  // Last used pad name (pad num)

PCB_SCREEN* ScreenModule  = NULL;

/* Options : */
// True to exclude contents of Edges Pcb layer
bool g_Exclude_Edges_Pcb = FALSE;
bool g_Plot_Frame_Ref;       // True to plot/print frame references
bool g_DrawViaOnMaskLayer;   // True if vias are drawn on Mask layer (ie protected by mask)

// = FILAIRE, FILL or SKETCH
int g_Plot_Mode = FILLED;
bool Plot_Set_MIROIR;
bool Sel_Rotate_Window;
bool HPGL_Org_Centre;       // TRUE si en HPGL, l'origine le centre de la feuille
int g_PlotPSColorOpt;       // True for color Postscript output
bool g_Plot_PS_Negative;    // True to create a  negative board ps plot

/* Autorisation de trace des divers items en serigraphie */
bool Sel_Texte_Reference = TRUE;
bool Sel_Texte_Valeur = TRUE;
bool Sel_Texte_Divers = TRUE;
bool Sel_Texte_Invisible;

/* Plot pads sur couche serigraphie */
bool PlotPadsOnSilkLayer = TRUE;
bool Plot_Pads_All_Layers; /* Plot pads meme n'appartenant pas a la
                              couche ( utile pour serigraphie) */


// Wildcard for footprint libraries filesnames
const wxString g_FootprintLibFileWildcard( wxT( "Kicad footprint library file (*.mod)|*.mod" ) );

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsability to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT("footprints_doc/footprints.pdf");

IMPLEMENT_APP( WinEDA_App )

/****************************/
bool WinEDA_App::OnInit()
/****************************/
{
    wxFileName fn;
    WinEDA_PcbFrame* frame = NULL;

    InitEDA_Appl( wxT( "PCBnew" ), APP_TYPE_PCBNEW );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    ScreenPcb = new PCB_SCREEN();
    GetSettings();

    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.GetExt() != BoardFileExtension )
        {
            wxLogDebug( wxT( "PcbNew file <%s> has the wrong extension.  \
Changing extension to .brd." ),
                        fn.GetFullPath().c_str() );
            fn.SetExt( BoardFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    wxGetApp().ReadCurrentSetupValues( ParamCfgList );
    g_DrawBgColor = BLACK;
    Read_Hotkey_Config( frame, false );  /* Must be called before creating the
                                          * main frame in order to display the
                                          * real hotkeys in menus or tool tips */


    /* allocation de la memoire pour le fichier et autres buffers: */
    /* On reserve BUFMEMSIZE octets de ram pour calcul */
    buf_work  = adr_lowmem = (char*) MyZMalloc( BUFMEMSIZE );   /* adresse de la zone de calcul */
    adr_max   = adr_lowmem;

    if( adr_lowmem == NULL )
    {
        printf( "No Memory, Fatal err Memory alloc\n" );
        return FALSE;
    }
    frame = new WinEDA_PcbFrame( NULL, wxT( "PcbNew" ),
                                 wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );
    ActiveScreen = ScreenPcb;

    SetTopWindow( frame );
    frame->Show( TRUE );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( TRUE );

    Read_Config( fn.GetFullPath() );

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        frame->LoadOnePcbFile( fn.GetFullPath(), FALSE );
        // update the layer names in the listbox
        frame->ReCreateLayerBox( NULL );
    }

    return TRUE;
}
