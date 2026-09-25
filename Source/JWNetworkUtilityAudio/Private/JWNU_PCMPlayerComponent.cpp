// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_PCMPlayerComponent.h"
#include "JWNU_AudioPCM.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UJWNU_PCMPlayerComponent::StartPlayer(int32 SampleRate)
{
    check(IsInGameThread());
    if (Output || !IsRegistered() || !GetOwner() || !GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer
        || !JWNU::AudioIO::IsSupportedSampleRate(SampleRate)) { return false; }
#if UE_SERVER
    return false;
#else
    Rate = SampleRate;
    Wave = NewObject<USoundWaveProcedural>(this);
    Wave->SetSampleRate(Rate); Wave->NumChannels = 1; Wave->SampleByteSize = 2;
    Wave->Duration = INDEFINITELY_LOOPING_DURATION; Wave->SoundGroup = SOUNDGROUP_Voice;
    Output = NewObject<UAudioComponent>(GetOwner());
    Output->bAutoActivate = false; Output->bAutoDestroy = false; Output->bAllowSpatialization = false;
    Output->bIsUISound = true; Output->bStopWhenOwnerDestroyed = true;
    Output->SetSound(Wave);
    Output->RegisterComponent();
    return true;
#endif
}
bool UJWNU_PCMPlayerComponent::QueuePCM(const TArray<uint8>& PCM16)
{
    check(IsInGameThread());
    if (!Output || !Wave || PCM16.IsEmpty() || PCM16.Num() % 2) { return false; }
    if (Wave->GetAvailableAudioByteCount() + static_cast<int64>(PCM16.Num()) > Rate * 4)
    {
        FJWNU_AudioError Error;
        Error.Code = TEXT("playback_overflow"); Error.Message = TEXT("Playback queue exceeded two seconds."); Error.bFatal = true;
        StopPlayer();
        OnErrorNative.Broadcast(Error); OnError.Broadcast(Error);
        return false;
    }
    Wave->QueueAudio(PCM16.GetData(), PCM16.Num());
    if (!Output->IsPlaying()) { Output->Play(); }
    return true;
}
float UJWNU_PCMPlayerComponent::GetBufferedSeconds() const
{
    return Wave ? static_cast<float>(Wave->GetAvailableAudioByteCount()) / (Rate * 2) : 0.f;
}
void UJWNU_PCMPlayerComponent::StopPlayer()
{
    check(IsInGameThread());
    if (Output) { Output->Stop(); Output->DestroyComponent(); Output = nullptr; }
    Wave = nullptr;
}
void UJWNU_PCMPlayerComponent::EndPlay(const EEndPlayReason::Type Reason) { StopPlayer(); Super::EndPlay(Reason); }
void UJWNU_PCMPlayerComponent::OnComponentDestroyed(bool bDestroyingHierarchy) { StopPlayer(); Super::OnComponentDestroyed(bDestroyingHierarchy); }
