# Make all of the output...
setwd("/storage/homes/michael/Documents/HPWH/HPWHsim/testTool/")

tests <- c("DOE_24hr50", "DOE_24hr67", "DP_SHW50")
models <- c("Voltex60", "ATI66", "GEred", "Sanden80", "GE2014")

# Simulate every combination of test and model
allResults <- do.call('rbind', lapply(tests, function(test) {
  do.call('rbind', lapply(models, function(model) {
    print(paste("Simulating model", model, "test", test))
    system(paste("./testTool.x", test, model))
    dset <- read.csv(paste("tests/", test, "/", model, "TestToolOutput.csv", sep = ""))
    dset$test <- test
    dset$model <- model
    dset
  }))
}))

# Convert all temperature variables from C to F
tempVars <- grep("Tcouple", names(allResults))
allResults[, tempVars] <- allResults[, tempVars] * 1.8 + 32
allResults$aveTankTemp <- apply(allResults[, tempVars], 1, mean)

inputVars <- grep("input_kWh", names(allResults))
allResults$inputTotal_W <- apply(allResults[, inputVars], 1, sum) * 60000
outputVars <- grep("output_kWh", names(allResults))
allResults$outputTotal_W <- apply(allResults[, outputVars], 1, sum) * 60000

allResults[] <- lapply(names(allResults), function(v) {
  if(length(grep("input_kWh", v)) | length(grep("output_kWh", v))) {
    NULL
  } else {
    allResults[, v]
  }
})


names(allResults)[names(allResults) == "draw"] <- "flow"
names(allResults)[names(allResults) == "simTcouples1"] <- "tcouples1"
names(allResults)[names(allResults) == "simTcouples2"] <- "tcouples2"
names(allResults)[names(allResults) == "simTcouples3"] <- "tcouples3"
names(allResults)[names(allResults) == "simTcouples4"] <- "tcouples4"
names(allResults)[names(allResults) == "simTcouples5"] <- "tcouples5"
names(allResults)[names(allResults) == "simTcouples6"] <- "tcouples6"
allResults$inputPower <- allResults$inputTotal_W
allResults$outputPower <- allResults$outputTotal_W

allResults <- allResults[, c("minutes", "test", "model", "flow", "inputPower", "outputPower",
                             "tcouples1", "tcouples2", "tcouples3",
                             "tcouples4", "tcouples5", "tcouples6",
                             "aveTankTemp")]
allResults$type <- "Simulated"

write.csv(file = "HpwhTestTool/allResults.csv", allResults, row.names = FALSE)
