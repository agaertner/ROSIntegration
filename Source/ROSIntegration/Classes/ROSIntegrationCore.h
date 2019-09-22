#pragma once

#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include <UObject/Object.h>

#include "ROSIntegrationCore.generated.h"

ROSINTEGRATION_API DECLARE_LOG_CATEGORY_EXTERN(LogROS, Display, All);


class USpawnManager;


class TDeleterNot
{
public:
	void operator()(void*) {}
};



// Wrapper of the private implementation of ROSIntegrationCore inside UObject 
UCLASS()
class ROSINTEGRATION_API UImpl : public UObject
{
	GENERATED_UCLASS_BODY()

protected:

	// PIMPL
	class Impl;
	TSharedPtr<Impl> impl_ = nullptr;

public: 

	UImpl();
	~UImpl();

	void Init();

	void BeginDestroy() override;

	void SetImplSpawnManager(USpawnManager* SpawnManager);

	FORCEINLINE TSharedPtr<Impl>& Get()
	{
		if (!impl_)
		{
			UE_LOG(LogROS, Error, TEXT("ROSIntegrationCore - UImpl empty"));
		}
		return impl_;
	}

};


UCLASS()
class ROSINTEGRATION_API UROSIntegrationCore : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	~UROSIntegrationCore();

	bool Init(FString ROSBridgeHost, int32 ROSBridgePort);

	bool IsHealthy() const;

	// You must call Init() before using this method to set upthe Implmentation correctly
	void SetWorld(UWorld* World);

	void InitSpawnManager();

	void BeginDestroy() override;

private:

	UPROPERTY()
	class UImpl* _Implementation = nullptr; // N.B: here there was the private implementation: a standard pointer which contained the USpawnManager.
											// This was manually added inside the GargageCollector by using the UObject::AddToRoot() method.
											//  AFAIU, after its creation, USpawnManager object was never destroyed. Now, with this change,
											//  its destruction should be automatically managed by the GargageCollector.

	UPROPERTY()
	USpawnManager* _SpawnManager = nullptr; // generated by this class and used by the _Implementation 

	bool _bson_test_mode = true;

	friend class UTopic;
	friend class UService;
};
