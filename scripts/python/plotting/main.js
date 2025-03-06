const sleep = ms => new Promise(r => setTimeout(r, ms));

	async function read_json_file(filename) {
		fst = 'filename=' + filename;
		const response = await fetch('http://localhost:8000/read_json?' + fst,
			{
				method: 'GET'
			}
		);
		json_data = await response.json();
		return json_data
	}

	async function write_json_file(filename, json_data) {
		fst = 'filename=' + filename;
		fst += '&json_data=' + JSON.stringify(json_data);
		await fetch('http://localhost:8000/write_json?' + fst,
			{
				method: 'GET'
			}
		);
	}

	async function callPyServer(cmd, fst) {
		await fetch('http://localhost:8000/' + cmd + '?' + fst);
	}

	async function callPyServerJSON(cmd, fst) {
		const response = await fetch('http://localhost:8000/' + cmd + '?' + fst);
		data = await response.json();
		return data;
	}

	async function init_elements() {
		var prefs = await read_json_file("./prefs.json")
		set_elements(prefs)

		params_table = document.getElementById('params_table');
		var rowCount = params_table.rows.length;
		for (var x=rowCount-1; x>0; x--) {
		   params_table.deleteRow(x);
		}

		//document.addEventListener("keydown", do_keydown)
	}

	let ws_connection = null;
	async function init_websocket() {
		await callPyServer("launch_ws", "")
		ws_connection = await new WebSocket("ws://localhost:8600");
		ws_connection.addEventListener("message", async (msg) => {
				if ('data' in msg)
				{
					const data = JSON.parse(msg['data']);
					if ('dest' in data)
						if(data['dest'] == "index")
							if ('cmd' in data)
							{
								if(!data['cmd'].localeCompare("init-test-proc"))									
								{
									let prefs = await get_menu_values();
									await set_elements(prefs);
									document.getElementById("test-plots").style = "display:block;"
								}

								if(!data['cmd'].localeCompare("init-perf-proc"))									
								{
									let prefs = await get_menu_values();
									await set_elements(prefs);
									document.getElementById("perf_plot").style = "display:block;"
								}

								if(!data['cmd'].localeCompare("refresh-fit"))
									FillFitTables()
							}
				
				}
			});
	}

	function set_menu_values(prefs) {
		const model_form = document.getElementById('model_form');
		const build_form = document.getElementById('build_form');
		const test_form = document.getElementById('test_form');

		model_form.model_name.value = ("model_id" in prefs) ? prefs["model_id"] : "LG_APHWC50"
		model_form.model_spec.value = ("model_spec" in prefs) ? prefs["model_spec"] : "Preset";
		build_form.build_dir.value = ("build_dir" in prefs) ? prefs["build_dir"] : "../../build";
		test_form.test_list.value = ("test_list" in prefs) ? prefs["test_list"] : "All tests";
		test_form.test_name.value = ("test_id" in prefs) ? prefs["test_id"] : "LG/APHWC50/DOE2014_LGC50_24hr67";
		test_form.draw_profile.value = ("draw_profile" in prefs) ? prefs["draw_profile"] : "auto";
	}

	async function get_menu_values() {
		const model_form = document.getElementById('model_form');
		const build_form = document.getElementById('build_form');
		const test_form = document.getElementById('test_form');

		prefs = await read_json_file("./prefs.json")
		prefs["model_id"] = model_form.model_name.value;
		prefs["model_spec"] = model_form.model_spec.value;
		prefs["build_dir"] = build_form.build_dir.value;
		prefs["test_list"] = test_form.test_list.value;
		prefs["test_id"] = test_form.test_name.value;
		prefs["draw_profile"] = test_form.draw_profile.value;
		return prefs;
	}

	async function set_elements(prefs) {
		const model_index = await read_json_file("./model_index.json")

		var model_names = []
		var model_specs = []

		// get model_specs list from model_index
		for (let model of model_index["models"]) {
			if ("id" in model) {
				const model_id = model["id"];
				model_names.push(model_id);
				if (!model_id.localeCompare(prefs["model_id"])) {
					if ("specs" in model)
						for (let i = 0; i < model["specs"].length; i++)
							model_specs.push(model["specs"][i]);
				}
			}
		}

		var model_id = prefs["model_id"];
		if (!(model_names.includes(model_id)))
			model_names = [model_names, model_id];

		if (!(model_specs.includes(prefs["model_spec"])))
			prefs["model_spec"] = model_specs[0];

		// set model_name control
		let select_model = document.getElementById('model_name');
		while (select_model.hasChildNodes()) {
			select_model.removeChild(select_model.firstChild);
		}
		for (let model_name of model_names) {
			let option = document.createElement("option");
			option.text = model_name;
			option.value = model_name;
			select_model.appendChild(option);
		}

		// set model_spec control
		let select_model_spec = document.getElementById('model_spec');
		while (select_model_spec.hasChildNodes()) {
			select_model_spec.removeChild(select_model_spec.firstChild);
		}
		for (let model_spec of model_specs) {
			let option = document.createElement("option");
			option.text = model_spec;
			option.value = model_spec;
			select_model_spec.appendChild(option);
		}

		// get available tests
		var have_test = false;

		const test_index = await read_json_file("./test_index.json")
		var test_names = []
		var show_all_tests = !prefs["test_list"].localeCompare("All tests");
		var show_tests_with_data = !prefs["test_list"].localeCompare("Tests with data");
		var show_standard_tests = show_all_tests || !prefs["test_list"].localeCompare("Standard tests");
		for (let test of test_index["tests"]) {
			var show_test = false;
			var test_name = "";
			if ("id" in test) {
				const test_id = test["id"];
				test_name = test_id;
				show_test = show_all_tests;
				if ("group" in test) {
					const test_group = test["group"];
					show_test |= show_standard_tests && !test_group.localeCompare("Standard tests");
					if (show_all_tests || show_tests_with_data)
						if ("measured" in test) {	 // get test list from model_index
							const measureds = test["measured"];
							for (let measured of measureds)
								if ("id" in measured)
									if (!measured["id"].localeCompare(prefs["model_id"])) {
										show_test = true;
										break;
									}
						}
				}

				if (show_test) {
					test_names.push(test_name);
					have_test = true;
				}
			}
		}

		if (have_test && !(test_names.includes(prefs["test_id"])))
			prefs["test_id"] = test_names[0];

		var have_measured = false;
		var measured_filename = "";
		var test_dir = "";
		for (let test of test_index["tests"]) {
			var test_id = "";
			if ("id" in test)
				test_id = test["id"];
			if (test_id == prefs["test_id"])
			{
				test_dir = test_id
				if ("path" in test)
					test_dir = test["path"] + "/" + test_id

				if ("measured" in test)
					for (let measured of test["measured"])
						if ("id" in measured)
							if (measured["id"] == prefs["model_id"]) {
								have_measured = true;
								if ('filename' in measured)
									measured_filename = measured['filename']
								break;
							}
			}
		}

		var is_standard_test = false;
		for (let test of test_index["tests"])
			if (("id" in test) && !test["id"].localeCompare(prefs["test_id"]))
				if (("group" in test) && !test["group"].localeCompare("Standard tests")) {
					is_standard_test = true;
					break;
				}

		// set select_test control
		let select_test = document.getElementById('test_name');
		while (select_test.hasChildNodes())
			select_test.removeChild(select_test.firstChild);

		prefs["show_simulated"] = have_test;
		prefs["show_measured"] = have_measured;

		if (!have_test) {
			prefs["test_id"] = "";
			select_test.disabled = true;
			document.getElementById('test_btn').disabled = true;
		}
		else {
			// set test_name control
			for (let test_name of test_names) {
				let option = document.createElement("option");
				option.text = test_name;
				option.value = test_name;
				select_test.appendChild(option);
			}

			select_test.disabled = false;
			document.getElementById('test_btn').disabled = false;
		}

		// set draw_profile dropdown
		let draw_profile_dropdown = document.getElementById('draw_profile');
		draw_profile_dropdown.value = prefs["draw_profile"];
		draw_profile_dropdown.disabled = !is_standard_test;

		set_menu_values(prefs);
		await write_json_file("./prefs.json", prefs)

		{// send perf info
			model_data_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
			var msg = {
				'source': 'index',
				'dest': 'perf-proc',
				'cmd': 'replot',
				'label': prefs['model_id'],
				'model_data_filepath': model_data_filepath};
			await ws_connection.send(JSON.stringify(msg));
		}

		var simulated_filepath = ""
		var measured_filepath = ""
		if (have_test)
		{ //measure or simulate
			if (is_standard_test) {
				const draw_profile = test_form.draw_profile.value;
				let data = {'model_spec': prefs["model_spec"], 'model_name': prefs["model_id"], 'build_dir': prefs['build_dir'], 'draw_profile': draw_profile}
				await callPyServer("measure", "data=" + JSON.stringify(data))
				simulated_filename = "test24hr_" + prefs["model_spec"] + "_" + prefs["model_id"] + ".csv";

				let res_path = prefs['build_dir'] + "/test/output/results.json"
			  var results = await read_json_file(res_path)
				document.getElementById("measure_results").innerHTML = JSON.stringify(results);
			}
			else {
				let data = {'model_spec': prefs["model_spec"], 'model_name': prefs["model_id"], 'build_dir': prefs['build_dir'], 'test_dir': test_dir};
				await callPyServer("simulate", "data=" + JSON.stringify(data))
				simulated_filename = prefs["test_id"] + "_" + prefs["model_spec"] + "_" + prefs["model_id"] + ".csv";
			}

		 // send test info
			var msg = {
				'source': 'index',
				'dest': 'test-proc',
				'cmd': 'replot',
				'build_dir': prefs['build_dir']};
			if (prefs["show_measured"])
			{
				measured_filepath = "../../../test"
				if(test_dir != "")
					measured_filepath += "/" + test_dir;
				measured_filepath += "/" + measured_filename;
				msg['measured_filepath'] = measured_filepath;
			}
			if (prefs["show_simulated"])
			{
				simulated_filepath = prefs['build_dir'] + "/test/output/" + simulated_filename;
				msg['simulated_filepath'] = simulated_filepath;
			}
			msg['is_standard_test'] = (is_standard_test ? 1 : 0);

			await ws_connection.send(JSON.stringify(msg));
		}
	}

	async function change_menu_value() {
		prefs = await get_menu_values();
		set_elements(prefs)
	}

	async function do_keydown(e)
	{
		// send test info
			console.log(e.key)
			var msg = {'source': 'index.html', 'dest': 'perf-proc', 'cmd': 'key-pressed', 'key': e.key};
			console.log(msg)
			await ws_connection.send(JSON.stringify(msg));
	}

	async function launch_test_proc() {

		document.getElementById("test_btn").disabled = true;
		var data = {};
		plot_results = await callPyServerJSON("launch_test_proc", "data=" + JSON.stringify(data));
		const dash_port = await plot_results["port_num"];

		document.getElementById("test-plots").src = "http://localhost:" + dash_port
		document.getElementById("test_btn").disabled = false;
	}

	async function launch_perf_proc() {
		document.getElementById("perf_btn").disabled = true;
		var data = {}
		let perf_results = await callPyServerJSON("launch_perf_proc", "data=" + JSON.stringify(data))
		const dash_port = perf_results["port_num"];

		document.getElementById("perf_plot").src = "http://localhost:" + dash_port
		document.getElementById("perf_btn").disabled = false;
	}

	async function clear_params() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['parameters'] = []
			await write_json_file("./fit_list.json", fit_list)
			await FillFitTables()
		}

	async function clear_data() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['data'] = []
			await write_json_file("./fit_list.json", fit_list)
			await FillFitTables()
		}

	async function FillParamsTable(fit_list) {	
		let tableHTML = ''

		have_point = false;
		if ('parameters' in fit_list)
		{		
			let params = fit_list['parameters']
			params.forEach(param =>
			{
				if ('type' in param)
					if(!param['type'].localeCompare('perf-point'))
					{
						const tableHeaders = Object.keys(param);
						if (!have_point)
						{
							tableHTML = '<table><thead><tr>';
							tableHeaders.forEach(header => {
							    tableHTML += `<th>${header}</th>`;
							  });
							tableHTML += '</tr></thead><tbody>';
							have_point = true;	
						}
						
				    tableHTML += '<tr>';
				    tableHeaders.forEach(header => {
				      tableHTML += `<td>${param[header] || ''}</td>`; 
				    });
				    tableHTML += '</tr>';
				  
					}
			});
		}
		if (!have_point)
			tableHTML = '<div>No parameters.</div>'
		document.getElementById('params_table').innerHTML = tableHTML;
	}

	async function FillDataTable(fit_list) {
		let tableHTML = ''

		have_point = false;
		if ('data' in fit_list)
		{		
			let data = fit_list['data']
			data.forEach(datum =>
			{		
				if ('type' in datum)
				{
					const tableHeaders = ['type', 'model_id', 'target'];
					if (!have_point)
					{
						tableHTML = '<table><thead><tr>';
						tableHeaders.forEach(header => {
						    tableHTML += `<th>${header}</th>`;
						  });
						tableHTML += '</tr></thead><tbody>';
						have_point = true;	
					}
						
					tableHTML += '<tr>';
					tableHeaders.forEach(header => {
						tableHTML += `<td>${datum[header] || ''}</td>`; 
					});
				   tableHTML += '</tr>';
				  have_point = true;
				}
			});
		}
		if (!have_point)
			tableHTML = '<div>No data.</div>'
		document.getElementById('data_table').innerHTML = tableHTML;
	}

	async function FillFitTables() {

		var fit_list = await read_json_file("./fit_list.json");

		await FillParamsTable(fit_list);
		await FillDataTable(fit_list);
	}