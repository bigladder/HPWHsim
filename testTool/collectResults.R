

setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")


collectLabData <- function(make) {
  print(paste("Collecting lab data for", make))
  # Read all of the actual results...
  files <- dir(paste("models", make, sep = "/"), pattern = "Full.csv", full.names = TRUE)
  labResults <- do.call('rbind', lapply(files, function(f) {
    tmp <- read.csv(f)
    test <- gsub("^.+/(.+)_(.+)_Full.csv$", "\\1_\\2", f)
    tmp$test <- test
    tmp$model <- make
    if(length(grep("1hr", test))) {
      tmp <- tmp[1:60, ]
    }
    tmp
  }))
  
  # Clean up the naming convention & variables
  names(labResults)[names(labResults) == "flow_out_gal"] <- "flow"
  names(labResults)[names(labResults) == "T_Tank_02"] <- "tcouples1"
  names(labResults)[names(labResults) == "T_Tank_04"] <- "tcouples2"
  names(labResults)[names(labResults) == "T_Tank_06"] <- "tcouples3"
  names(labResults)[names(labResults) == "T_Tank_08"] <- "tcouples4"
  names(labResults)[names(labResults) == "T_Tank_10"] <- "tcouples5"
  names(labResults)[names(labResults) == "T_Tank_12"] <- "tcouples6"
  labResults$inputPower <- labResults$Power_W
  labResults$outputPower <- labResults$dEadd5min / 5 / 60 * 1000
  
  tempVars <- grep("tcouples", names(labResults))
  # labResults[, tempVars] <- labResults[, tempVars] * 1.8 + 32
  labResults$aveTankTemp <- apply(labResults[, tempVars], 1, mean)
  labResults$Ta <- labResults$T_Plenum_In * 1.8 + 32
  
  labResults <- labResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                               "tcouples1", "tcouples2", "tcouples3",
                               "tcouples4", "tcouples5", "tcouples6",
                               "aveTankTemp", "Ta", "COP5min")]
  labResults$type <- "Measured"
  labResults
}

collectSimData <- function(make) {
  print(paste("Collecting simulated data for", make))
  # Read all of the simulated results...
  files <- dir(paste("models", make, sep = "/"), pattern = "TestToolOutput.csv",
               recursive = TRUE, full.names = TRUE)
  if(!length(files)) return(NULL)
  simResults <- do.call('rbind', lapply(files, function(f) {
    tmp <- read.csv(f)
    test <- gsub("^.+/(.+)_(.+)/TestToolOutput.csv$", "\\1_\\2", f)
    tmp$test <- test
    tmp$model <- make
    if(length(grep("1hr", test))) {
      tmp <- tmp[1:60, ]
    }
    tmp
  }))
  
  # Clean up the sim results
  # Parse the energy outputs
  inputVars <- grep("input_kWh", names(simResults))
  if(length(inputVars) > 1) {
    simResults$inputTotal_W <- apply(simResults[, inputVars], 1, sum) * 60000
  } else {
    simResults$inputTotal_W <- simResults[, inputVars] * 60000
  }
  simResults <- simResults[, -inputVars]
  
  outputVars <- grep("output_kWh", names(simResults))
  if(length(outputVars) > 1) {
    simResults$outputTotal_W <- apply(simResults[, outputVars], 1, sum) * 60000
  } else {
    simResults$outputTotal_W <- simResults[, outputVars] * 60000
  }
  simResults <- simResults[, -outputVars]
  
  tempVars <- grep("Tcouple", names(simResults))
  simResults[, tempVars] <- simResults[, tempVars] * 1.8 + 32
  simResults$aveTankTemp <- apply(simResults[, tempVars], 1, mean)
  
  
  names(simResults)[names(simResults) == "draw"] <- "flow"
  names(simResults)[names(simResults) == "simTcouples1"] <- "tcouples1"
  names(simResults)[names(simResults) == "simTcouples2"] <- "tcouples2"
  names(simResults)[names(simResults) == "simTcouples3"] <- "tcouples3"
  names(simResults)[names(simResults) == "simTcouples4"] <- "tcouples4"
  names(simResults)[names(simResults) == "simTcouples5"] <- "tcouples5"
  names(simResults)[names(simResults) == "simTcouples6"] <- "tcouples6"
  simResults$inputPower <- simResults$inputTotal_W
  simResults$outputPower <- simResults$outputTotal_W
  
  simResults <- simResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                               "tcouples1", "tcouples2", "tcouples3",
                               "tcouples4", "tcouples5", "tcouples6",
                               "aveTankTemp")]
  simResults$type <- "Simulated"
  simResults
}

makes <- c("AOSmith60", "AOSmith80",
           "AOSmithHPTU50", "AOSmithHPTU66", "AOSmithHPTU80",
           "GEred", "GE502014", "GE502014STDMode", "RheemHB50", 
           "SandenGAU", "Stiebel220e")

labData <- do.call('rbind', lapply(makes, collectLabData))
simData <- do.call('rbind', lapply(makes, collectSimData))

write.csv(file = "HpwhTestTool/labResults.csv", labData, row.names = FALSE)
write.csv(file = "HpwhTestTool/simResults.csv", simData, row.names = FALSE)




