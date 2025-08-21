#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, juce::ChangeListener, private juce::Timer
{
public:
  

    MainComponent();
    ~MainComponent() override;
    

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    
    void timerCallback() override
    {
        repaint();
    }

    
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;

    juce::String currentFileName;
    

private:
    
    enum TransportState {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };
    
    void openButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    
    void thumbnailChanged();
    void changeState(TransportState state);
    
    void transportSourceChanged();
    
    void paintIfNoFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds);
    
    void paintIfFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds);
   // juce::Text
    
    
    //This class contains a list of audio formats (such as WAV, AIFF, Ogg Vorbis, and so on) and can create suitable objects for reading audio data from these formats.
    juce::AudioFormatManager formatManager;
    
    //This is a subclass of the AudioSource class. It can read audio data from an AudioFormatReader object and render the audio via its getNextAudioBlock() function.
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    
  //  can control the playback of an AudioFormatReaderSource object. This control includes starting and stopping the playback of the AudioFormatReaderSource object.
    juce::AudioTransportSource transportSource;
    

    
    TransportState state;
    
    std::unique_ptr<juce::FileChooser> chooser;
    
    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail; 



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
