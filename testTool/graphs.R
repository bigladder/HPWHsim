# Code to make some official graphs...

# setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/HpwhTestTool")
source("global.R")

dir.create("graphs")
# Voltex 60 gallon
p <- onePlot("Voltex60", "DOE_24hr50", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 8) +
  ggtitle("Voltex 60 Gallon DOE 24hr50 Test")
ggsave(file = "graphs/DOE_24hr50_Voltex60.png", p, width = 7, height = 5)

p <- onePlot("Voltex60", "DP_SHW50", c("Thermocouples", "Average Tank Temp", "Input Power"), 
             tmin = 0, tmax = 200 / 60) +
  ggtitle("Voltex 60 Gallon Shower Test")
ggsave(file = "graphs/DP_SHW5050_Voltex60.png", p, width = 7, height = 5)


px <- fieldPlot("Voltex60")
px <- px + ggtitle("Voltex 60 gallon Field vs HPWHsim 2.0")
ggsave(file = "graphs/voltex60field.png", px, width = 7, height = 5)


# The GE Geospring!!!
px <- fieldPlot("GEred")
px <- px + ggtitle("GE GeoSpring Field Data vs HPWHsim 2.0")
ggsave(file = "graphs/gefield.png", px, width = 7, height = 5)

p <- onePlot("GEred", "DOE_24hr50", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 24)
p
ggsave(file = "graphs/DOE_24hr50_GEred.png", p, width = 7, height = 5)

p <- onePlot("GEred", "DOE_24hr50", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 9)
p
ggsave(file = "graphs/DOE_24hr50_GEred2.png", p, width = 7, height = 5)

p <- onePlot("GEred", "DOE_24hr67", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 24)
p
ggsave(file = "graphs/DOE_24hr67_GEred.png", p, width = 7, height = 5)

p <- onePlot("GEred", "DOE_24hr67", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 8)
p
ggsave(file = "graphs/DOE_24hr67_GEred2.png", p, width = 7, height = 5)

p <- onePlot("GEred", "DP_SHW50", c("Thermocouples", "Average Tank Temp", "Input Power"), tmin = 0, tmax = 24)
p
ggsave(file = "graphs/DP_SHW50_GEred.png", p, width = 7, height = 5)