#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
:state (Stopped) , thumbnailCache (5), thumbnail (512, formatManager, thumbnailCache)
{
    
    openButton.setButtonText("Open");
    addAndMakeVisible(&openButton);
    openButton.onClick = [this] { openButtonClicked(); };
    
    
    
    stopButton.setButtonText("Stop");
    addAndMakeVisible(&stopButton);
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled (false);
    
    
    playButton.setButtonText("Play");
    addAndMakeVisible(&playButton);
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled (false);
    
    thumbnail.addChangeListener (this); // [6]
    
    
    
    setSize (600, 400);
    
    transportSource.addChangeListener (this);
    
    formatManager.registerBasicFormats();
    
    setAudioChannels(0, 2);
    
    startTimer (40);

    
    
}



void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Select an audio file to play...",
        juce::File {},
        "*.wav;*.mp3;*.aiff;*.flac"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File{}) return;

        // 🔧 Clean handover: stop and detach old source
        transportSource.stop();
        transportSource.setSource (nullptr);
        readerSource.reset();

        if (auto* reader = formatManager.createReaderFor (file))
        {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
            readerSource = std::move(newSource);

            currentFileName = file.getFileName();
            thumbnail.setSource (new juce::FileInputSource (file));
            playButton.setEnabled (true);

      
            changeState (Stopped);
        }
        else
        {
            // Couldn’t open the file; disable play to be safe
            playButton.setEnabled (false);
        }
    });
    
    
}

MainComponent::~MainComponent(){
    // Stop playback and detach sources BEFORE shutting down audio
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();
    transportSource.removeChangeListener(this);

    // Now close the audio device; this will stop the audio thread
    shutdownAudio();
};

void MainComponent::playButtonClicked()
{
    if ((state == Stopped) || (state == Paused))
        changeState (Starting);
    else if (state == Playing)
        changeState (Pausing);
    
    
};

void MainComponent::stopButtonClicked(){
    
    if (state == Paused)
         changeState (Stopped);
     else
         changeState (Stopping);
    
};




void MainComponent::changeState (TransportState newState)
{
    if (state != newState)
    {
        state = newState;
        switch (state)
        {
            case Stopped:
                playButton.setButtonText ("Play");
                stopButton.setButtonText ("Stop");
                stopButton.setEnabled (false);
                transportSource.setPosition (0.0);
                break;
            case Starting:
                transportSource.start();
                break;
            case Playing:
                playButton.setButtonText ("Pause");
                stopButton.setButtonText ("Stop");
                stopButton.setEnabled (true);
                break;
            case Pausing:
                transportSource.stop();
                break;
            case Paused:
                playButton.setButtonText ("Resume");
                stopButton.setButtonText ("Return to Zero");
                break;
            case Stopping:
                transportSource.stop();
                break;
        }
    }
}



void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState (Playing);
        else if ((state == Stopping) || (state == Playing))
            changeState (Stopped);
        else if (Pausing == state)
            changeState (Paused);
    }

    if (source == &thumbnail){
        thumbnailChanged();
    }
    
    
}

void MainComponent::thumbnailChanged()
{
    repaint();
}

void MainComponent::transportSourceChanged()
{
    changeState (transportSource.isPlaying() ? Playing : Stopped);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    transportSource.getNextAudioBlock (bufferToFill);
}




void MainComponent::resized(){
    openButton.setBounds (10, 10, getWidth() - 20, 20);
    playButton.setBounds (10, 40, getWidth() - 20, 20);
    stopButton.setBounds (10, 70, getWidth() - 20, 20);

};

void MainComponent::paint (juce::Graphics& g)
{
    // Fill background with default window colour
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Set colour and font, then draw some text
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    
    
    if(currentFileName.isEmpty()){
        g.drawText ("Audio Player", getLocalBounds(),
                    juce::Justification::centred, true);
    }
    else {
        g.drawText (currentFileName, getLocalBounds(),
                    juce::Justification::centred, true);
    }
    
    
    
    juce::Rectangle<int> thumbnailBounds (10, 100, getWidth() - 20, getHeight() - 120);
    if (thumbnail.getNumChannels() == 0)
        paintIfNoFileLoaded(g, thumbnailBounds);
    else
        paintIfFileLoaded (g, thumbnailBounds);
    
}


void MainComponent::paintIfFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour (juce::Colours::black);
    g.fillRect (thumbnailBounds);
    g.setColour (juce::Colours::green); // [8]
    thumbnail.drawChannels (g, // [9]
        thumbnailBounds,
        0.0, // start time
        thumbnail.getTotalLength(), // end time
        1.0f); // vertical zoom
}

void MainComponent::paintIfNoFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
{
    g.setColour (juce::Colours::black);
    g.fillRect (thumbnailBounds);
    g.setColour (juce::Colours::black);
    g.drawFittedText ("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
}




void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate){
    
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
};


void MainComponent::releaseResources(){
    
    transportSource.releaseResources();
};


