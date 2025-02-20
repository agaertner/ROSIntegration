// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ROSIntegrationCore.h"

#include "ROSBridgeConnection.generated.h"


UCLASS()
class ROSINTEGRATION_API AROSBridgeConnection : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "rosbridge")
	FString host = "127.0.0.1";

	UPROPERTY(EditAnywhere, Category = "rosbridge")
	int port = 9090;

	UPROPERTY(EditAnywhere, Category = "rosbridge")
	EROSConnType connType = EROSConnType::TCPConn;

protected:
	UPROPERTY()
	UROSIntegrationCore* ROSIntegrationCore = nullptr;

public:	
	AROSBridgeConnection();

	UROSIntegrationCore* GetCore()
	{
		return ROSIntegrationCore;
	}

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

};
