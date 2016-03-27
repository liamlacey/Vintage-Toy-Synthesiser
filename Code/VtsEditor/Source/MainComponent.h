/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

//FIXME: all backend stuff (MIDI IO, patch data storage, saving and loading files) should ideally be done in a seperate class to this one.
//Right now I just want a quick and dirty working application though!


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   :  public Component,
                                public MidiInputCallback,
                                public Button::Listener,
                                public FileBrowserListener
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&);
    void resized();
    
    void buttonClicked (Button *button);
    
    void handleIncomingMidiMessage (MidiInput *source, const MidiMessage &message);
    
    //FileBrowserListener pure virtual callback functions
    void selectionChanged();
    void fileClicked (const File &file, const MouseEvent &e);
    void fileDoubleClicked (const File &file);
    void browserRootChanged (const File &newRoot);
    
    void sendMidiMessage (MidiMessage midiMessage);
    
    void savePatchFile ();
    void loadPatchFile (const File patch_file);

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
    
    //GUI
    ScopedPointer <AudioDeviceSelectorComponent> audioDeviceSelectorComponent;
    
    ScopedPointer<TextButton> requestPatchDataButton;
    
    ScopedPointer<Label> patchNameEditor;
    ScopedPointer<TextButton> saveButton;
    ScopedPointer<FileListComponent> patchDirFileListComponent;
    
    ScopedPointer<TextButton> resetSoundEngineButton;
    ScopedPointer<TextButton> disablePanelButton;
    
    ScopedPointer<Label> eventLabel;
    
    //backend
    AudioDeviceManager audioDeviceManager;
    
    int patchData[127];
    File patchDir;
    
    TimeSliceThread timeSliceThread;
    
    ScopedPointer <WildcardFileFilter> patchFileFilter;
    ScopedPointer <DirectoryContentsList> patchFileDirContentsList;
    
};


#endif  // MAINCOMPONENT_H_INCLUDED
