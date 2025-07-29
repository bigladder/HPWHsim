
	const gui = {};
	gui.perf_proc_active = false;
	gui.test_proc_active = false;


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

	//
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
								await get_menu_values();
								await set_elements();
							}

							if(!data['cmd'].localeCompare("init-perf-proc"))									
							{
								await get_menu_values();
								await set_elements();
							}

							if(!data['cmd'].localeCompare("refresh-fit"))
								FillFitTables()
						}			
				}
			});
	}

	//
	async function init_elements() {
		prefs = await read_json_file("./prefs.json")

		// model drop-down
		const models = await read_json_file("./model_index.json")
		var model_ids = []
		for (model_id in models)
			model_ids.push(model_id);

		if (!prefs['model_id'] in models)
			model_ids.push(prefs['model_id']);

		let select_model = document.getElementById('model_id');
		while (select_model.hasChildNodes()) {
			select_model.removeChild(select_model.firstChild);
		}
		for (let model_id of model_ids) {
			let option = document.createElement("option");
			option.text = model_id;
			option.value = model_id;
			select_model.appendChild(option);
		}
		select_model.value = prefs['model_id']

		params_table = document.getElementById('params_table');
		var rowCount = params_table.rows.length;
		for (var x=rowCount-1; x>0; x--) {
		   params_table.deleteRow(x);
		}
		await set_elements();
	}

	async function set_menu_values() {
		const model_form = document.getElementById('model_form');
		const build_form = document.getElementById('build_form');
		const test_form = document.getElementById('test_form');

		prefs = await read_json_file("./prefs.json")
		model_form.model_id.value = ("model_id" in prefs) ? prefs["model_id"] : "LG_APHWC50"
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
		prefs["model_id"] = model_form.model_id.value;
		prefs["build_dir"] = build_form.build_dir.value;
		prefs["test_list"] = test_form.test_list.value;
		prefs["test_id"] = test_form.test_name.value;
		prefs["draw_profile"] = test_form.draw_profile.value;
		await write_json_file("./prefs.json", prefs);
	}

	async function replot_performance(model_filepath) 
	{// send perf info
		var msg = {
			'source': 'index',
			'dest': 'perf-proc',
			'cmd': 'replot',
			'model_filepath': model_filepath
		};
		await ws_connection.send(JSON.stringify(msg));
	}

	async function replot_test(measured_filepath, simulated_filepath, is_standard_test)
	{
			prefs = await read_json_file("./prefs.json")
			var msg = {
				'source': 'index',
				'dest': 'test-proc',
				'cmd': 'replot'};
			if (prefs["show_measured"])
			{
				msg['measured_filepath'] = measured_filepath;
			}
			if (prefs["show_simulated"])
			{
					msg['simulated_filepath'] = simulated_filepath;
			}
			msg['is_standard_test'] = (is_standard_test ? 1 : 0);

			await ws_connection.send(JSON.stringify(msg));
	}

	async function run_standard_test()
	{ //measure
		prefs = await read_json_file("./prefs.json")
		await callPyServer("measure", "data=" + JSON.stringify(prefs))
	}

	async function run_general_test()
	{ // simulate
		prefs = await read_json_file("./prefs.json")
		await callPyServer("simulate", "data=" + JSON.stringify(prefs))
	}

	async function change_model()
	{
		get_menu_values();
		set_elements();
	}

	async function set_test_info()
	{
		prefs = await read_json_file("./prefs.json")

		// get available tests
		const test_index = await read_json_file("./test_index.json")
		var show_all_tests = !prefs["test_list"].localeCompare("All tests");
		var show_tests_with_data = !prefs["test_list"].localeCompare("Tests with data");
		var show_standard_tests = show_all_tests || !prefs["test_list"].localeCompare("Standard tests");
		var test_names = [];
		var first = true;
		var test_info = {};
		for (let [test_id, test_data] in test_index["tests"]) {
			var show_test = false;
			show_test = show_all_tests;
			if ('group' in test_data) {
				const test_group = test_data['group'];
				show_test |= show_standard_tests && !test_group.localeCompare("Standard tests");
				if (show_all_tests || show_tests_with_data)
					if ('measured' in test_data)	 // check for measured
							if (test_id in test_data['measured'])
								show_test = true;
			}
			const test_name = ('name' in test_info)? test_info['name'] : test_id;
			if (show_test)
				test_names.push(test_name);

			if (first || (test_id.localeCompare(prefs['test_id'])))
			{
				test_info = test_data;
				first = false;
			}
		}
	
		// get available tests
		if (!test_names.includes(prefs["test_id"]));
			prefs["test_id"] = test_names[0]; // default
	
		test_info['test_dir'] = prefs["test_id"]
		for (let [test_id, test_info] in test_index["tests"]) {
			if (!test_id.localeCompare(prefs["test_id"]))
			{
				test_info['test_dir'] = (("path" in test_info)? test_info["path" ] + "/": "") + prefs["test_id"];		
				if ("measured" in test_info)
					for (let [model_id, filename] of test["measured"])
						if (model_id == prefs["model_id"]) {
						{
							test_info['measured_filename'] = measured['filename'];
							prefs["show_measured"] = true;
							break;
						}
					}
			}
		}

		// set add'l test info
		for (let [test_id, test_info] in test_index["tests"])  {
			if (("group" in test_info) && !test_info["group"].localeCompare("Standard tests")) {
				test_info['is_standard_test'] = true;
				test_info['simulated_filename'] = "test24hrEF_" + prefs["model_id"] + ".csv";
				prefs["show_simulated"] = true;
				break;
			}
			else
			{
				test_info['simulated_filename'] = prefs["test_id"] + "_JSON_" + prefs["model_id"] + ".csv";
				prefs["show_simulated"] = true;
				break;
			}	
		}

		// update select_test control
		let select_test = document.getElementById('test_name');
		while (select_test.hasChildNodes())
			select_test.removeChild(select_test.firstChild);

			// set test_name control
		for (let test_name of test_names) {
			let option = document.createElement("option");
			option.text = test_name;
			option.value = test_name;
			select_test.appendChild(option);
		}
		if (test_names.length > 0)
		{ 
			select_test.disabled = false;
			document.getElementById('test_btn').disabled = false;
		}
		else
		{
			prefs["test_id"] = "";
			select_test.disabled = true;
			document.getElementById('test_btn').disabled = true;
		}

		// set draw_profile dropdown
		let draw_profile_dropdown = document.getElementById('draw_profile');
		draw_profile_dropdown.value = prefs["draw_profile"];
		draw_profile_dropdown.disabled = !('is_standard_test' in test_info);

		await write_json_file("./prefs.json", prefs)
	}

	async function set_elements() {
		var prefs = await read_json_file("./prefs.json")
	
		test_info = await set_test_info()

		set_menu_values();

		var model_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
		if (gui.perf_proc_active)
			await replot_performance(model_filepath)

		if (gui.test_proc_active)
		{
			if('is_standard_test' in test_info)
				await run_standard_test()
			else
			{
				await run_general_test(test_info['test_dir'])
			}
			
			var measured_filepath = ""
			if (prefs["show_measured"])
			{
				measured_filepath = "../../../test"
				if ('test_dir' in test_info)
					measured_filepath += "/" + test_info['test_dir'];
				measured_filepath += "/" + test_info['measured_filename'];
			}

			var simulated_filepath = ""
			if (prefs["show_simulated"])
				simulated_filepath += prefs['build_dir'] + "/test/output/" + test_info['simulated_filename'];

			let is_standard = ('is_standard_test' in test_info);
			await replot_test(measured_filepath, simulated_filepath, is_standard)
		}
		await write_json_file("./prefs.json", prefs)
	}

	async function change_menu_value() {
		await get_menu_values();
		await set_elements();
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
		if (gui.test_proc_active)
		{
			document.getElementById("test_plot").src = "";
			document.getElementById("test_btn").innerHTML = "show";
			document.getElementById("test_plot").style="display:none;"
			gui.test_proc_active = false;
		}
		else
		{
			var data = {};
			plot_results = await callPyServerJSON("launch_test_proc", "data=" + JSON.stringify(data));
			const dash_port = await plot_results["port_num"];
			document.getElementById("test_plot").src = "http://localhost:" + dash_port;
			document.getElementById("test_btn").innerHTML = "hide";
			document.getElementById("test_plot").style = "display:block;"
			gui.test_proc_active = true;
		}
		await set_elements();
		document.getElementById("test_btn").disabled = false;
	}

	async function launch_perf_proc() {
		document.getElementById("perf_btn").disabled = true;
		if (gui.perf_proc_active)
		{
			document.getElementById("perf_plot").src = "";
			document.getElementById("perf_btn").innerHTML = "show";
			document.getElementById("perf_plot").style="display:none;"
			gui.perf_proc_active = false;
		}
		else
		{
			var data = {}
			let perf_results = await callPyServerJSON("launch_perf_proc", "data=" + JSON.stringify(data))
			const dash_port = perf_results["port_num"];
			document.getElementById("perf_plot").src = "http://localhost:" + dash_port
			document.getElementById("perf_btn").innerHTML = "hide";
			document.getElementById("perf_plot").style = "display:block;"
			gui.perf_proc_active = true;
		}
		await set_elements();
		document.getElementById("perf_btn").disabled = false;
	}

	async function launch_fit_proc() {
		document.getElementById("fit_btn").disabled = true;
		var data = {}
		let fit_results = await callPyServerJSON("launch_fit_proc", "data=" + JSON.stringify(data))
		document.getElementById("fit_btn").disabled = false;
	}

	async function clear_params() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['parameters'] = []
			await write_json_file("./fit_list.json", fit_list)
			await FillFitTables()
		}

	async function clear_metrics() {
			var fit_list = await read_json_file("./fit_list.json")
			fit_list['metrics'] = {}
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

	async function FillMetricsTable(fit_list) {
		let tableHTML = '<table>';
		if ('metrics' in fit_list)
		{		
			let metrics = fit_list['metrics']
			have_metric = false;
			if('test_points' in metrics)
			{
				const tableHeaders = ['model_id', 'test_id', 't_min', 'variable', 'value'];
				for(const test_point of metrics['test_points'])
				{
					if (!have_point)
					{
						tableHTML += '<thead><tr>';
						tableHeaders.forEach(header => {
						    tableHTML += `<th>${header}</th>`;
						  });
						tableHTML += '</tr></thead><tbody>';
						have_point = true;	
					}
						
					tableHTML += '<tr>';
					tableHeaders.forEach(header => {
						tableHTML += `<td>${test_point[header] || ''}</td>`; 
					});
					tableHTML += '</tr>';
				  have_point = true;
				};
				tableHTML += '</tbody>';
				have_metric |= have_point;
			}

			have_energy_factor = false;
			if('energy_factors' in metrics)
			{
				const tableHeaders = ['model_id', 'draw_profile', 'value'];
				for (const energy_factor of metrics['energy_factors'])
				{
					if (!have_energy_factor)
					{
						tableHTML += '<thead><tr>';
						tableHeaders.forEach(header => {
						    tableHTML += `<th>${header}</th>`;
						  });
						tableHTML += '</tr></thead><tbody>';
						have_energy_factor = true;	
					}
						
					tableHTML += '<tr>';
					tableHeaders.forEach(header => {
						tableHTML += `<td>${energy_factor[header] || ''}</td>`; 
					});
				   tableHTML += '</tr>';
				  have_energy_factor = true;
				}
				tableHTML += '</tbody>';
				have_metric |= have_energy_factor;
			}
		}
		if (!have_metric)
			tableHTML = '<div>No metrics.</div>'
		document.getElementById('metrics_table').innerHTML = tableHTML;
	}

	async function FillFitTables() {

		var fit_list = await read_json_file("./fit_list.json");

		await FillParamsTable(fit_list);
		await FillMetricsTable(fit_list);
	}

	async function fit()	{

	}

	async function FillGeneralTable(model_data) {
		let tableHTML = '<table>'
		
		const tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		const tableEntries = ['number_of_nodes', 'standard_setpoint'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in model_data)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + model_data[entry] + '</td> </tr>';
		});
		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('general_table').innerHTML = tableHTML;
	}

	async function FillTankTable(model_data) {
		let tableHTML = '<table>'
		
		var tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		var perf;
		if("integrated_system" in model_data)
		{
			wh = model_data["integrated_system"];
			perf = wh["performance"];
		}
		else
			perf = model_data["central_system"]			

		let tank_perf = perf["tank"]["performance"]

		tableEntries = ['volume', 'ua', 'fittings_ua', 'bottom_fraction_of_tank_mixing_on_draw'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in tank_perf)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + tank_perf[entry] + '</td> </tr>';
		});
		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('tank_table').innerHTML = tableHTML;
	}

	async function FillHeatSourceTable(heat_source_config) {
		let tableHTML = '<table>'
			
		var tableHeaders = ['parameter', 'value'];
		tableHTML += '<thead><tr>';
		tableHeaders.forEach(header => {
		    tableHTML += `<th>${header}</th>`;
		  });
		tableHTML += '</tr></thead>';

		const tableEntries = ['heat_source_type'];
		tableHTML += '<tbody>';
		tableEntries.forEach(entry => {	
			if (entry in heat_source_config)
				tableHTML += '<tr> <th>' + entry + '</th> <td>' + heat_source_config[entry] + '</td> </tr>';
		});

		let heat_source = heat_source_config["heat_source"];
		let heat_source_perf = heat_source["performance"]

		tableHTML += '</tbody>';

		tableHTML += '</table>';
		document.getElementById('heat_source_table').innerHTML = tableHTML;
	}

	async function FillPropertiesTables() {
		prefs = await read_json_file("./prefs.json")
		const model_data_filepath = "../../../test/models_json/" + prefs['model_id'] + ".json";
		var model_data = await read_json_file(model_data_filepath)

		await FillGeneralTable(model_data);
		await FillTankTable(model_data);

		var performance;
		if("integrated_system" in model_data)
		{
			wh = model_data["integrated_system"];
			performance = wh["performance"];
		}
		else
			performance = model_data["central_system"]	;		
		performance["heat_source_configurations"].forEach(hsc => {	
			if (hsc["id"] == prefs["heat_source_id"])
			{
					FillHeatSourceTable(hsc);
			}
		}
		);
	}

async function init_all() {
	await init_websocket();
	await init_elements();
	await FillFitTables();
	await FillPropertiesTables();
}