library(reshape2)
library(ggplot2)

simData4_1 <- read.csv("./DrawProfileTest_4p1_24hr67/TestToolOutput.csv")
attr(simData4_1, "source")  <- "4 person - Day 1"
simData4_2 <- read.csv("./DrawProfileTest_4p2_24hr67/TestToolOutput.csv")
attr(simData4_2, "source")  <- "4 person - Day 2"
simData4_3 <- read.csv("./DrawProfileTest_4p3_24hr67/TestToolOutput.csv")
attr(simData4_3, "source")  <- "4 person - Day 3"
simData4_4 <- read.csv("./DrawProfileTest_4p4_24hr67/TestToolOutput.csv")
attr(simData4_4, "source")  <- "4 person - Day 4"
simData4_5 <- read.csv("./DrawProfileTest_4p5_24hr67/TestToolOutput.csv")
attr(simData4_5, "source")  <- "4 person - Day 5"
simData4_6 <- read.csv("./DrawProfileTest_4p6_24hr67/TestToolOutput.csv")
attr(simData4_6, "source")  <- "4 person - Day 6"
simData4_7 <- read.csv("./DrawProfileTest_4p7_24hr67/TestToolOutput.csv")
attr(simData4_7, "source")  <- "4 person - Day 7"

simData5_1 <- read.csv("./DrawProfileTest_5p1_24hr67/TestToolOutput.csv")
attr(simData5_1, "source")  <- "5 person - Day 1"
simData5_2 <- read.csv("./DrawProfileTest_5p2_24hr67/TestToolOutput.csv")
attr(simData5_2, "source")  <- "5 person - Day 2"
simData5_3 <- read.csv("./DrawProfileTest_5p3_24hr67/TestToolOutput.csv")
attr(simData5_3, "source")  <- "5 person - Day 3"
simData5_4 <- read.csv("./DrawProfileTest_5p4_24hr67/TestToolOutput.csv")
attr(simData5_4, "source")  <- "5 person - Day 4"
simData5_5 <- read.csv("./DrawProfileTest_5p5_24hr67/TestToolOutput.csv")
attr(simData5_5, "source")  <- "5 person - Day 5"
simData5_6 <- read.csv("./DrawProfileTest_5p6_24hr67/TestToolOutput.csv")
attr(simData5_6, "source")  <- "5 person - Day 6"
simData5_7 <- read.csv("./DrawProfileTest_5p7_24hr67/TestToolOutput.csv")
attr(simData5_7, "source")  <- "5 person - Day 7"

testlist <- list(simData4_1, simData4_2, simData4_3, simData4_4, simData4_5, simData4_6, simData4_7, simData5_1, simData5_2, simData5_3, simData5_4, simData5_5, simData5_6, simData5_7)

lapply(testlist, function(simData){
  simData$inputPower <- (simData$input_kWh1 + simData$input_kWh2 + simData$input_kWh3) * 60000
  simData$outputPower <- (simData$output_kWh1 + simData$output_kWh2 + simData$output_kWh3) * 60000
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

