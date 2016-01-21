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

