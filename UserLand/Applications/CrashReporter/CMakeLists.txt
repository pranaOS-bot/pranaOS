compile_gml(CrashReporterWindow.gml CrashReporterWindowGML.h crash_reporter_window_gml)


set(SOURCES
    main.cpp
    CrashReporterWindowGML.h
)

pranaos_app(CrashReporter ICON app-crash-reporter)
target_link_libraries(CrashReporter LibCore LibCoreDump LibDesktop LibGUI)