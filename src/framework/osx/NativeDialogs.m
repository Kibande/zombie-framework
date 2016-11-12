
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

static char s_pathBuffer[4096];

int NatDlg_ErrorWindow(const char* path, int is_fatal)
{
    NSAlert* alert = [NSAlert new];

    [alert setAlertStyle: NSCriticalAlertStyle];
    [alert setMessageText: @"Application Error"];
    [alert setInformativeText: [NSString stringWithUTF8String: path]];

    [alert addButtonWithTitle: @"No"];
    [alert addButtonWithTitle: @"Yes"];

#ifdef DEBUG
    [alert addButtonWithTitle: @"Debug"];
#endif

    int rc = 0;

    switch([alert runModal])
    {
        case NSAlertThirdButtonReturn: rc = 2; break;
        case NSAlertSecondButtonReturn: rc = 1; break;
        case NSAlertFirstButtonReturn: rc = 0; break;
    }

    [alert release];
    return rc;
}

const char* NatDlg_FileOpen(const char* defaultDir, const char* defaultFileName)
{
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:NO];
    
    if ([openDlg runModal] == NSOKButton)
    {
        NSArray* urls = [openDlg URLs];

        if ([urls count] > 0)
        {
            NSURL* url = [urls objectAtIndex:0];
            
            s_pathBuffer[0] = 0;
            [[url path] getCString:s_pathBuffer maxLength:sizeof(s_pathBuffer) encoding:NSUTF8StringEncoding];
            
            [urls release];
            [openDlg release];
            return s_pathBuffer;
        }
        
        [urls release];
    }
    
    [openDlg release];
    return 0;
}

const char* NatDlg_FileSaveAs(const char* defaultDir, const char* defaultFileName)
{
    NSSavePanel* saveDlg = [NSSavePanel savePanel];
    
    if ([saveDlg runModal] == NSOKButton)
    {
        NSURL* url = [saveDlg URL];
            
        s_pathBuffer[0] = 0;
        [[url path] getCString:s_pathBuffer maxLength:sizeof(s_pathBuffer) encoding:NSUTF8StringEncoding];
        
        [saveDlg release];
        return s_pathBuffer;
    }
    
    [saveDlg release];
    return 0;
}

void NatDlg_ShellOpenFile(const char* path)
{
    [[NSWorkspace sharedWorkspace] openFile:[NSString stringWithUTF8String: path]];
}
