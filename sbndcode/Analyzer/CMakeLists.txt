art_make(MODULE_LIBRARIES
        nusimdata_SimulationBase
	lardataobj_RecoBase
	larsim_Utils

        ${ART_FRAMEWORK_SERVICES_REGISTRY}
        ${ART_ROOT_IO_TFILE_SUPPORT} 
        ${ART_ROOT_IO_TFILESERVICE_SERVICE}

        ${ROOT_BASIC_LIB_LIST}

	sbndcode_GeoWrappers
	)

install_headers()
install_fhicl()
install_source()
