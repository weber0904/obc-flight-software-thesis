module OBCComCcsds {

    enum UhfPorts_ComPacketQueue : U8 {
        EVENTS,
        HANDSHAKE,
        TELEMETRY
    }

    enum UhfPorts_ComBufferQueue : U8 {
        FILE
    }

    instance comQueue: Svc.ComQueue base id OBCComCcsdsConfig.BASE_ID + 0x00000 \
        queue size OBCComCcsdsConfig.QueueSizes.comQueue \
        stack size OBCComCcsdsConfig.StackSizes.comQueue \
        priority OBCComCcsdsConfig.Priorities.comQueue \
    {
        phase Fpp.ToCpp.Phases.configComponents """
        using namespace OBCComCcsds;
        Svc::ComQueue::QueueConfigurationTable uhfConfigurationTable;

        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::EVENTS].depth = OBCComCcsdsConfig::QueueDepths::events;
        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::EVENTS].priority = OBCComCcsdsConfig::QueuePriorities::events;

        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::HANDSHAKE].depth =
            OBCComCcsdsConfig::QueueDepths::handshake;
        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::HANDSHAKE].priority =
            OBCComCcsdsConfig::QueuePriorities::handshake;

        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::TELEMETRY].depth = OBCComCcsdsConfig::QueueDepths::tlm;
        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::TELEMETRY].priority = OBCComCcsdsConfig::QueuePriorities::tlm;

        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::NUM_CONSTANTS + UhfPorts_ComBufferQueue::FILE].depth =
            OBCComCcsdsConfig::QueueDepths::file;
        uhfConfigurationTable.entries[UhfPorts_ComPacketQueue::NUM_CONSTANTS + UhfPorts_ComBufferQueue::FILE].priority =
            OBCComCcsdsConfig::QueuePriorities::file;

        OBCComCcsds::comQueue.configure(uhfConfigurationTable, 0, OBCComCcsds::Allocation::memAllocator);
        """
        phase Fpp.ToCpp.Phases.tearDownComponents """
        OBCComCcsds::comQueue.cleanup();
        """
    }

    instance frameAccumulator: Svc.FrameAccumulator base id OBCComCcsdsConfig.BASE_ID + 0x01000 \
    {
        phase Fpp.ToCpp.Phases.configObjects """
        Svc::FrameDetectors::CcsdsTcFrameDetector frameDetector;
        """
        phase Fpp.ToCpp.Phases.configComponents """
        OBCComCcsds::frameAccumulator.configure(
            ConfigObjects::OBCComCcsds_frameAccumulator::frameDetector,
            1,
            OBCComCcsds::Allocation::memAllocator,
            OBCComCcsdsConfig::BuffMgr::frameAccumulatorSize
        );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
        OBCComCcsds::frameAccumulator.cleanup();
        """
    }

    instance commsBufferManager: Svc.BufferManager base id OBCComCcsdsConfig.BASE_ID + 0x02000 \
    {
        phase Fpp.ToCpp.Phases.configObjects """
        Svc::BufferManager::BufferBins bins;
        """

        phase Fpp.ToCpp.Phases.configComponents """
        memset(&ConfigObjects::OBCComCcsds_commsBufferManager::bins, 0,
               sizeof(ConfigObjects::OBCComCcsds_commsBufferManager::bins));
        ConfigObjects::OBCComCcsds_commsBufferManager::bins.bins[0].bufferSize = OBCComCcsdsConfig::BuffMgr::commsBuffSize;
        ConfigObjects::OBCComCcsds_commsBufferManager::bins.bins[0].numBuffers = OBCComCcsdsConfig::BuffMgr::commsBuffCount;
        ConfigObjects::OBCComCcsds_commsBufferManager::bins.bins[1].bufferSize = OBCComCcsdsConfig::BuffMgr::commsFileBuffSize;
        ConfigObjects::OBCComCcsds_commsBufferManager::bins.bins[1].numBuffers = OBCComCcsdsConfig::BuffMgr::commsFileBuffCount;
        OBCComCcsds::commsBufferManager.setup(
            OBCComCcsdsConfig::BuffMgr::commsBuffMgrId,
            0,
            OBCComCcsds::Allocation::memAllocator,
            ConfigObjects::OBCComCcsds_commsBufferManager::bins
        );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
        OBCComCcsds::commsBufferManager.cleanup();
        """
    }

    instance fprimeRouter: Svc.FprimeRouter base id OBCComCcsdsConfig.BASE_ID + 0x03000

    instance tcDeframer: Svc.Ccsds.TcDeframer base id OBCComCcsdsConfig.BASE_ID + 0x04000 {
        phase Fpp.ToCpp.Phases.configComponents """
        OBCComCcsds::tcDeframer.configure(2U, ComCfg::SpacecraftId, false);
        """
    }

    instance spacePacketDeframer: Svc.Ccsds.SpacePacketDeframer base id OBCComCcsdsConfig.BASE_ID + 0x05000

    instance aggregator: Svc.ComAggregator base id OBCComCcsdsConfig.BASE_ID + 0x06000 \
        queue size OBCComCcsdsConfig.QueueSizes.aggregator \
        stack size OBCComCcsdsConfig.StackSizes.aggregator

    instance vcidAdapter: OBC.UhfCcsdsVcidAdapter base id OBCComCcsdsConfig.BASE_ID + 0x06800

    instance framer: Svc.Ccsds.TmFramer base id OBCComCcsdsConfig.BASE_ID + 0x07000

    instance spacePacketFramer: Svc.Ccsds.SpacePacketFramer base id OBCComCcsdsConfig.BASE_ID + 0x08000

    instance apidManager: Svc.Ccsds.ApidManager base id OBCComCcsdsConfig.BASE_ID + 0x09000

    instance comStub: Svc.ComStub base id OBCComCcsdsConfig.BASE_ID + 0x0A000

    topology UhfFramingSubtopology {
        instance comQueue
        instance commsBufferManager
        instance frameAccumulator
        instance fprimeRouter
        instance tcDeframer
        instance spacePacketDeframer
        instance framer
        instance spacePacketFramer
        instance apidManager
        instance aggregator
        instance vcidAdapter

        connections Downlink {
            comQueue.dataOut -> spacePacketFramer.dataIn
            spacePacketFramer.dataReturnOut -> comQueue.dataReturnIn

            spacePacketFramer.bufferAllocate -> commsBufferManager.bufferGetCallee
            spacePacketFramer.bufferDeallocate -> commsBufferManager.bufferSendIn
            spacePacketFramer.getApidSeqCount -> apidManager.getApidSeqCountIn

            spacePacketFramer.dataOut -> aggregator.dataIn
            aggregator.dataOut -> vcidAdapter.dataIn
            vcidAdapter.dataOut -> framer.dataIn

            framer.dataReturnOut -> vcidAdapter.dataReturnIn
            vcidAdapter.dataReturnOut -> aggregator.dataReturnIn
            aggregator.dataReturnOut -> spacePacketFramer.dataReturnIn

            framer.comStatusOut -> vcidAdapter.comStatusIn
            vcidAdapter.comStatusOut -> aggregator.comStatusIn
            aggregator.comStatusOut -> spacePacketFramer.comStatusIn
            spacePacketFramer.comStatusOut -> comQueue.comStatusIn
        }

        connections Uplink {
            frameAccumulator.bufferDeallocate -> commsBufferManager.bufferSendIn
            frameAccumulator.bufferAllocate -> commsBufferManager.bufferGetCallee

            frameAccumulator.dataOut -> tcDeframer.dataIn
            tcDeframer.dataReturnOut -> frameAccumulator.dataReturnIn

            tcDeframer.dataOut -> spacePacketDeframer.dataIn
            spacePacketDeframer.dataReturnOut -> tcDeframer.dataReturnIn

            spacePacketDeframer.validateApidSeqCount -> apidManager.validateApidSeqCountIn

            spacePacketDeframer.dataOut -> fprimeRouter.dataIn
            fprimeRouter.dataReturnOut -> spacePacketDeframer.dataReturnIn

            fprimeRouter.bufferAllocate -> commsBufferManager.bufferGetCallee
            fprimeRouter.bufferDeallocate -> commsBufferManager.bufferSendIn
        }
    }

    topology UhfSubtopology {
        import UhfFramingSubtopology

        instance comStub

        connections ComStub {
            OBCComCcsds.framer.dataOut -> comStub.dataIn
            comStub.dataReturnOut -> OBCComCcsds.framer.dataReturnIn
            comStub.comStatusOut -> OBCComCcsds.framer.comStatusIn

            comStub.dataOut -> OBCComCcsds.frameAccumulator.dataIn
            OBCComCcsds.frameAccumulator.dataReturnOut -> comStub.dataReturnIn
        }
    }
}
