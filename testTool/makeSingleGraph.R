library(reshape2)
library(ggplot2)

setwd("./tests")

simData67 <- read.csv("./DOE_24hr67/TestToolOutput.csv")
attr(simData67, "source")  <- "24 hour 67 Sanden 40b"
simData50 <- read.csv("./DOE_24hr50/TestToolOutput.csv")
attr(simData50, "source")  <- "24 hour 50 Sanden 40b"


testlist <- list(simData67, simData50)

lapply(testlist, function(simData){
  simData$inputPower <- (simData$input_kWh1 ) * 60000
  simData$outputPower <- (simData$output_kWh1 ) * 60000
  simData$aveTankTemp <- (simData$simTcouples1 + simData$simTcouples2 + simData$simTcouples3 + simData$simTcouples4 + simData$simTcouples5 + simData$simTcouples6)/6
  simData_m <- melt(simData, id = "minutes")
  
  
  varGuide <- data.frame("variable" = c("draw", "inputPower", "outputPower",
                                        "simTcouples1", "simTcouples2", "simTcouples3",
                                        "simTcouples4", "simTcouples5", "simTcouples6",
                                        "aveTankTemp", "Ta"),
                         "units" = c("Gallons", rep("Watts", 2),
                                     rep("Fahrenheit", 8)),
                         "category" = c("Draw", "Input Power", "Output Power",
                                        rep("Thermocouples", 6), "Average Tank Temp",
                                        "Ambient Temp"))
  
  simData_m <- merge(simData_m, varGuide)
  
  ggplot(data = simData_m, aes(x = minutes, y = value, group = variable, col = variable)) + theme_bw() + 
    geom_line() + facet_grid(units~., scales = "free_y") +
    ggtitle(attr(simData, "source"))

  ggsave(filename = paste0(attr(simData, "source"), ".png"))
  })

